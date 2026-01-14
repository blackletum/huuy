//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Moderate texture streaming system implementation
//
//=============================================================================

#include "pch_materialsystem.h"

#define MATSYS_INTERNAL

#include "texturestreaming.h"
#include "texturemanager.h"
#include "itextureinternal.h"
#include "vtf/vtf.h"
#include "filesystem.h"
#include "tier0/vprof.h"
#include "tier1/convar.h"
#include "materialsystem/imaterialsystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// ConVars for runtime tuning
//-----------------------------------------------------------------------------
static ConVar mat_streaming_enabled( "mat_streaming_enabled", "1", FCVAR_ARCHIVE, "Enable texture streaming" );
static ConVar mat_streaming_budget_mb( "mat_streaming_budget_mb", "200", FCVAR_ARCHIVE, "Memory budget for streamed textures in MB" );
static ConVar mat_streaming_resident_mips( "mat_streaming_resident_mips", "4", FCVAR_ARCHIVE, "Number of mip levels always kept in memory" );
static ConVar mat_streaming_max_concurrent( "mat_streaming_max_concurrent", "8", FCVAR_ARCHIVE, "Maximum concurrent texture loads" );
static ConVar mat_streaming_debug( "mat_streaming_debug", "0", 0, "Print streaming debug info" );

//-----------------------------------------------------------------------------
// Singleton
//-----------------------------------------------------------------------------
CTextureStreamingManager* CTextureStreamingManager::s_pInstance = NULL;

CTextureStreamingManager* CTextureStreamingManager::GetInstance()
{
	if ( !s_pInstance )
	{
		s_pInstance = new CTextureStreamingManager();
	}
	return s_pInstance;
}

//-----------------------------------------------------------------------------
// Constructor/Destructor
//-----------------------------------------------------------------------------
CTextureStreamingManager::CTextureStreamingManager()
	: m_StreamingTextures( DefLessFunc( ITextureInternal* ) )
	, m_bShutdown( false )
	, m_nCurrentFrame( 0 )
{
	m_nCurrentMemoryBytes = 0;
}

CTextureStreamingManager::~CTextureStreamingManager()
{
	Shutdown();
}

//-----------------------------------------------------------------------------
// Initialization
//-----------------------------------------------------------------------------
void CTextureStreamingManager::Init( const TextureStreamingConfig_t* pConfig )
{
	AUTO_LOCK( m_Mutex );
	
	if ( pConfig )
	{
		m_Config = *pConfig;
	}
	
	// Apply ConVar overrides
	m_Config.bEnabled = mat_streaming_enabled.GetBool();
	m_Config.nStreamingBudgetMB = mat_streaming_budget_mb.GetInt();
	m_Config.nResidentMipLevels = mat_streaming_resident_mips.GetInt();
	m_Config.nMaxConcurrentLoads = mat_streaming_max_concurrent.GetInt();
	
	// Create scratch VTF pool
	for ( int i = 0; i < m_Config.nMaxConcurrentLoads; i++ )
	{
		IVTFTexture* pVTF = CreateVTFTexture();
		m_ScratchVTFPool.PushItem( pVTF );
	}
	
	// Start worker threads
	if ( m_Config.bEnabled )
	{
		StartWorkerThreads();
	}
	
	Msg( "[TextureStreaming] Initialized: budget=%dMB, resident_mips=%d, workers=%d\n",
		m_Config.nStreamingBudgetMB, m_Config.nResidentMipLevels, m_WorkerThreads.Count() );
}

void CTextureStreamingManager::Shutdown()
{
	StopWorkerThreads();
	
	AUTO_LOCK( m_Mutex );
	
	// Clean up scratch VTFs
	IVTFTexture* pVTF = NULL;
	while ( m_ScratchVTFPool.PopItem( &pVTF ) )
	{
		DestroyVTFTexture( pVTF );
	}
	
	// Clean up any pending jobs
	StreamingLoadJob_t* pJob = NULL;
	while ( m_JobQueue.PopItem( &pJob ) )
	{
		delete pJob;
	}
	while ( m_CompletedJobs.PopItem( &pJob ) )
	{
		delete pJob;
	}
	
	m_StreamingTextures.RemoveAll();
	m_PendingRequests.RemoveAll();
	m_LoadingTextures.RemoveAll();
	
	if ( s_pInstance == this )
	{
		s_pInstance = NULL;
	}
}

void CTextureStreamingManager::SetConfig( const TextureStreamingConfig_t& config )
{
	AUTO_LOCK( m_Mutex );
	m_Config = config;
}

//-----------------------------------------------------------------------------
// Worker threads
//-----------------------------------------------------------------------------
void CTextureStreamingManager::StartWorkerThreads()
{
	m_bShutdown = false;
	
	// Create worker threads based on CPU cores (but reasonable limit)
	int nNumWorkers = MIN( 4, (int)GetCPUInformation()->m_nPhysicalProcessors / 2 );
	nNumWorkers = MAX( 1, nNumWorkers );
	
	for ( int i = 0; i < nNumWorkers; i++ )
	{
		ThreadHandle_t hThread = CreateSimpleThread( LoaderThreadMain, this );
		if ( hThread )
		{
			m_WorkerThreads.AddToTail( hThread );
		}
	}
}

void CTextureStreamingManager::StopWorkerThreads()
{
	m_bShutdown = true;
	
	// Wait for all workers to finish
	for ( int i = 0; i < m_WorkerThreads.Count(); i++ )
	{
		ThreadJoin( m_WorkerThreads[i] );
	}
	m_WorkerThreads.RemoveAll();
}

uintp CTextureStreamingManager::LoaderThreadMain( void* pParam )
{
	ThreadSetDebugName( "TexStreamLoader" );
	CTextureStreamingManager* pManager = (CTextureStreamingManager*)pParam;
	pManager->LoaderThread_Main();
	return 0;
}

void CTextureStreamingManager::LoaderThread_Main()
{
	while ( !m_bShutdown )
	{
		StreamingLoadJob_t* pJob = NULL;
		
		if ( m_JobQueue.PopItem( &pJob ) )
		{
			LoaderThread_ProcessJob( pJob );
			m_CompletedJobs.PushItem( pJob );
		}
		else
		{
			// No work available, sleep a bit
			ThreadSleep( 4 );
		}
	}
}

void CTextureStreamingManager::LoaderThread_ProcessJob( StreamingLoadJob_t* pJob )
{
	if ( !pJob || !pJob->pTexture || !pJob->pScratchVTF )
		return;
	
	VPROF_BUDGET( "TextureStreaming::LoadJob", "TextureStreaming" );
	
	// Read texture data from disk into scratch VTF
	// This is the CPU-intensive part that benefits from threading
	pJob->pTexture->AsyncReadTextureFromFile( pJob->pScratchVTF, pJob->nAdditionalFlags );
}

//-----------------------------------------------------------------------------
// Main update - call once per frame
//-----------------------------------------------------------------------------
void CTextureStreamingManager::Update()
{
	if ( !m_Config.bEnabled )
		return;
	
	VPROF_BUDGET( "TextureStreaming::Update", "TextureStreaming" );
	
	m_nCurrentFrame++;
	
	// Process completed loads (must be on main thread for GPU upload)
	ProcessCompletedLoads();
	
	// Start new load jobs
	{
		AUTO_LOCK( m_Mutex );
		
		// Sort pending requests by priority
		// Higher priority and older requests first
		for ( int i = 0; i < m_PendingRequests.Count() - 1; i++ )
		{
			for ( int j = i + 1; j < m_PendingRequests.Count(); j++ )
			{
				bool bSwap = false;
				
				if ( m_PendingRequests[i].nPriority < m_PendingRequests[j].nPriority )
					bSwap = true;
				else if ( m_PendingRequests[i].nPriority == m_PendingRequests[j].nPriority &&
						  m_PendingRequests[i].nRequestFrame > m_PendingRequests[j].nRequestFrame )
					bSwap = true;
				
				if ( bSwap )
				{
					TextureStreamRequest_t temp = m_PendingRequests[i];
					m_PendingRequests[i] = m_PendingRequests[j];
					m_PendingRequests[j] = temp;
				}
			}
		}
		
		// Start loading textures up to our concurrent limit
		while ( m_LoadingTextures.Count() < m_Config.nMaxConcurrentLoads &&
				m_PendingRequests.Count() > 0 )
		{
			TextureStreamRequest_t request = m_PendingRequests[0];
			m_PendingRequests.Remove( 0 );
			
			// Get a scratch VTF from the pool
			IVTFTexture* pScratchVTF = NULL;
			if ( !m_ScratchVTFPool.PopItem( &pScratchVTF ) )
			{
				// No scratch VTF available, put request back
				m_PendingRequests.InsertBefore( 0, request );
				break;
			}
			
			// Create load job
			StreamingLoadJob_t* pJob = new StreamingLoadJob_t();
			pJob->pTexture = request.pTexture;
			pJob->pScratchVTF = pScratchVTF;
			pJob->nTargetMipLevel = request.nTargetMipLevel;
			pJob->bHighPriority = ( request.nPriority > 0 );
			
			// Update texture info
			StreamingTextureInfo_t* pInfo = FindTextureInfo( request.pTexture );
			if ( pInfo )
			{
				pInfo->residenceState = TEXTURE_RESIDENCE_LOADING;
				pInfo->nLoadStartFrame = m_nCurrentFrame;
			}
			
			m_LoadingTextures.AddToTail( request.pTexture );
			m_JobQueue.PushItem( pJob );
		}
	}
	
	// Handle evictions if over budget
	UpdateEvictions();
}

void CTextureStreamingManager::ProcessCompletedLoads()
{
	StreamingLoadJob_t* pJob = NULL;
	
	while ( m_CompletedJobs.PopItem( &pJob ) )
	{
		if ( pJob && pJob->pTexture )
		{
			// Upload to GPU (must be on main thread)
			// The scratch VTF has the loaded data
			pJob->pTexture->Download( NULL, pJob->nAdditionalFlags );
			
			// Update texture info
			AUTO_LOCK( m_Mutex );
			
			StreamingTextureInfo_t* pInfo = FindTextureInfo( pJob->pTexture );
			if ( pInfo )
			{
				pInfo->residenceState = TEXTURE_RESIDENCE_FULL;
				pInfo->nCurrentMipLevel = 0;
				pInfo->nCurrentSizeBytes = pInfo->nFullSizeBytes;
				pInfo->nLastUsedFrame = m_nCurrentFrame;
			}
			
			// Remove from loading list
			m_LoadingTextures.FindAndRemove( pJob->pTexture );
			
			// Return scratch VTF to pool
			if ( pJob->pScratchVTF )
			{
				m_ScratchVTFPool.PushItem( pJob->pScratchVTF );
			}
			
			if ( mat_streaming_debug.GetBool() )
			{
				Msg( "[TextureStreaming] Loaded: %s\n", pJob->pTexture->GetName() );
			}
		}
		
		delete pJob;
	}
}

void CTextureStreamingManager::UpdateEvictions()
{
	int nBudgetBytes = m_Config.nStreamingBudgetMB * 1024 * 1024;
	
	if ( m_nCurrentMemoryBytes <= nBudgetBytes )
		return;
	
	AUTO_LOCK( m_Mutex );
	
	// Build list of eviction candidates
	CUtlVector<StreamingTextureInfo_t*> candidates;
	
	FOR_EACH_MAP_FAST( m_StreamingTextures, i )
	{
		StreamingTextureInfo_t& info = m_StreamingTextures[i];
		
		// Only evict fully loaded textures that haven't been used recently
		if ( info.residenceState == TEXTURE_RESIDENCE_FULL &&
			 m_nCurrentFrame - info.nLastUsedFrame > (uint32)m_Config.nEvictionDelayFrames )
		{
			candidates.AddToTail( &info );
		}
	}
	
	// Sort by last used frame (oldest first)
	for ( int i = 0; i < candidates.Count() - 1; i++ )
	{
		for ( int j = i + 1; j < candidates.Count(); j++ )
		{
			if ( candidates[i]->nLastUsedFrame > candidates[j]->nLastUsedFrame )
			{
				StreamingTextureInfo_t* temp = candidates[i];
				candidates[i] = candidates[j];
				candidates[j] = temp;
			}
		}
	}
	
	// Evict until under budget
	for ( int i = 0; i < candidates.Count() && m_nCurrentMemoryBytes > nBudgetBytes; i++ )
	{
		EvictTexture( candidates[i]->pTexture );
	}
}

void CTextureStreamingManager::EvictTexture( ITextureInternal* pTexture )
{
	StreamingTextureInfo_t* pInfo = FindTextureInfo( pTexture );
	if ( !pInfo || pInfo->residenceState != TEXTURE_RESIDENCE_FULL )
		return;
	
	// Calculate size reduction
	int nResidentMip = CalculateResidentMipLevel( 
		pTexture->GetActualWidth(), 
		pTexture->GetActualHeight() );
	
	// Tell texture to drop high-res mips
	// This requires ITextureInternal to support partial unload
	// For now we'll mark it and handle on next load request
	pInfo->residenceState = TEXTURE_RESIDENCE_PARTIAL;
	pInfo->nCurrentMipLevel = nResidentMip;
	
	// Calculate number of mip levels from dimensions
	int nWidth = pTexture->GetActualWidth();
	int nHeight = pTexture->GetActualHeight();
	int nMipCount = 1;
	while ( nWidth > 1 || nHeight > 1 )
	{
		nWidth = MAX( 1, nWidth >> 1 );
		nHeight = MAX( 1, nHeight >> 1 );
		nMipCount++;
	}
	
	// Update memory tracking
	int nNewSize = CalculateTextureSizeBytes(
		pTexture->GetActualWidth() >> nResidentMip,
		pTexture->GetActualHeight() >> nResidentMip,
		nMipCount - nResidentMip,
		pTexture->GetImageFormat() );
	
	m_nCurrentMemoryBytes -= ( pInfo->nCurrentSizeBytes - nNewSize );
	pInfo->nCurrentSizeBytes = nNewSize;
	
	if ( mat_streaming_debug.GetBool() )
	{
		Msg( "[TextureStreaming] Evicted: %s (saved %d KB)\n", 
			pTexture->GetName(), 
			( pInfo->nFullSizeBytes - nNewSize ) / 1024 );
	}
}

//-----------------------------------------------------------------------------
// Texture loading interface - COMPATIBILITY LAYER
//-----------------------------------------------------------------------------
bool CTextureStreamingManager::ShouldStreamTexture( ITextureInternal* pTexture, int nWidth, int nHeight, int nFlags )
{
	if ( !m_Config.bEnabled )
		return false;
	
	return IsTextureEligibleForStreaming( pTexture, nWidth, nHeight, nFlags );
}

bool CTextureStreamingManager::IsTextureEligibleForStreaming( ITextureInternal* pTexture, int nWidth, int nHeight, int nFlags )
{
	// Don't stream render targets
	if ( nFlags & TEXTUREFLAGS_RENDERTARGET )
		return false;
	
	// Don't stream procedural textures
	if ( nFlags & TEXTUREFLAGS_PROCEDURAL )
		return false;
	
	// Don't stream textures that explicitly disable streaming
	if ( nFlags & TEXTUREFLAGS_NOLOD )
		return false;
	
	// Don't stream cubemaps (for simplicity)
	if ( nFlags & TEXTUREFLAGS_ENVMAP )
		return false;
	
	// Don't stream small textures
	if ( nWidth <= m_Config.nMinTextureSize && nHeight <= m_Config.nMinTextureSize )
		return false;
	
	return true;
}

bool CTextureStreamingManager::LoadTexturePartial( ITextureInternal* pTexture, int nAdditionalFlags )
{
	if ( !pTexture )
		return false;
	
	int nWidth = pTexture->GetActualWidth();
	int nHeight = pTexture->GetActualHeight();
	
	// Calculate which mip level to load as "resident"
	int nResidentMip = CalculateResidentMipLevel( nWidth, nHeight );
	
	// If texture is small enough, just load everything
	if ( nResidentMip <= 0 )
	{
		pTexture->Download( NULL, nAdditionalFlags );
		return true;
	}
	
	AUTO_LOCK( m_Mutex );
	
	// Create tracking info
	StreamingTextureInfo_t* pInfo = GetOrCreateTextureInfo( pTexture );
	pInfo->nCurrentMipLevel = nResidentMip;
	pInfo->residenceState = TEXTURE_RESIDENCE_PARTIAL;
	
	// Calculate number of mip levels from dimensions
	int nMipCount = 1;
	int w = nWidth, h = nHeight;
	while ( w > 1 || h > 1 )
	{
		w = MAX( 1, w >> 1 );
		h = MAX( 1, h >> 1 );
		nMipCount++;
	}
	
	// Calculate sizes
	pInfo->nFullSizeBytes = CalculateTextureSizeBytes( 
		nWidth, nHeight, 
		nMipCount,
		pTexture->GetImageFormat() );
	
	pInfo->nCurrentSizeBytes = CalculateTextureSizeBytes(
		nWidth >> nResidentMip,
		nHeight >> nResidentMip,
		nMipCount - nResidentMip,
		pTexture->GetImageFormat() );
	
	m_nCurrentMemoryBytes += pInfo->nCurrentSizeBytes;
	
	// Load only resident mips
	// This requires modifying ITextureInternal to support partial load
	// For now, we load everything but track that we could evict
	pTexture->Download( NULL, nAdditionalFlags | TEXTUREFLAGS_STREAMABLE );
	
	// Mark as fully loaded for now (until we implement partial download)
	pInfo->residenceState = TEXTURE_RESIDENCE_FULL;
	pInfo->nCurrentMipLevel = 0;
	pInfo->nCurrentSizeBytes = pInfo->nFullSizeBytes;
	
	if ( mat_streaming_debug.GetBool() )
	{
		Msg( "[TextureStreaming] Initial load: %s (%dx%d, %d KB)\n",
			pTexture->GetName(), nWidth, nHeight, pInfo->nFullSizeBytes / 1024 );
	}
	
	return true;
}

void CTextureStreamingManager::RequestFullResolution( ITextureInternal* pTexture )
{
	if ( !m_Config.bEnabled || !pTexture )
		return;
	
	AUTO_LOCK( m_Mutex );
	
	StreamingTextureInfo_t* pInfo = FindTextureInfo( pTexture );
	if ( !pInfo )
		return;
	
	// Update last used time
	pInfo->nLastUsedFrame = m_nCurrentFrame;
	
	// If already full or loading, nothing to do
	if ( pInfo->residenceState == TEXTURE_RESIDENCE_FULL ||
		 pInfo->residenceState == TEXTURE_RESIDENCE_LOADING )
	{
		return;
	}
	
	// Check if already in pending queue
	for ( int i = 0; i < m_PendingRequests.Count(); i++ )
	{
		if ( m_PendingRequests[i].pTexture == pTexture )
		{
			// Boost priority
			m_PendingRequests[i].nPriority++;
			return;
		}
	}
	
	// Queue load request
	QueueLoadRequest( pTexture, 0, false );
}

void CTextureStreamingManager::ForceFullLoad( ITextureInternal* pTexture )
{
	if ( !pTexture )
		return;
	
	StreamingTextureInfo_t* pInfo = FindTextureInfo( pTexture );
	
	// If not tracked or already full, just do a normal download
	if ( !pInfo || pInfo->residenceState == TEXTURE_RESIDENCE_FULL )
	{
		return;
	}
	
	// Synchronous load - blocks until complete
	pTexture->Download( NULL, 0 );
	
	AUTO_LOCK( m_Mutex );
	
	if ( pInfo )
	{
		pInfo->residenceState = TEXTURE_RESIDENCE_FULL;
		pInfo->nCurrentMipLevel = 0;
		pInfo->nCurrentSizeBytes = pInfo->nFullSizeBytes;
		pInfo->nLastUsedFrame = m_nCurrentFrame;
	}
}

void CTextureStreamingManager::ReleaseFullResolution( ITextureInternal* pTexture )
{
	// Just mark for potential eviction - actual eviction happens in Update()
	AUTO_LOCK( m_Mutex );
	
	StreamingTextureInfo_t* pInfo = FindTextureInfo( pTexture );
	if ( pInfo )
	{
		pInfo->bPendingEviction = true;
	}
}

//-----------------------------------------------------------------------------
// Query interface
//-----------------------------------------------------------------------------
TextureResidenceState_t CTextureStreamingManager::GetTextureResidence( ITextureInternal* pTexture ) const
{
	AUTO_LOCK( m_Mutex );
	
	StreamingTextureInfo_t* pInfo = const_cast<CTextureStreamingManager*>(this)->FindTextureInfo( pTexture );
	if ( !pInfo )
		return TEXTURE_RESIDENCE_NONE;
	
	return pInfo->residenceState;
}

bool CTextureStreamingManager::IsTextureFullyLoaded( ITextureInternal* pTexture ) const
{
	return GetTextureResidence( pTexture ) == TEXTURE_RESIDENCE_FULL;
}

int CTextureStreamingManager::GetCurrentMemoryUsageMB() const
{
	return m_nCurrentMemoryBytes / ( 1024 * 1024 );
}

//-----------------------------------------------------------------------------
// Lifecycle callbacks
//-----------------------------------------------------------------------------
void CTextureStreamingManager::OnTextureDestroyed( ITextureInternal* pTexture )
{
	AUTO_LOCK( m_Mutex );
	
	// Remove from tracking
	int idx = m_StreamingTextures.Find( pTexture );
	if ( idx != m_StreamingTextures.InvalidIndex() )
	{
		m_nCurrentMemoryBytes -= m_StreamingTextures[idx].nCurrentSizeBytes;
		m_StreamingTextures.RemoveAt( idx );
	}
	
	// Remove from pending requests
	for ( int i = m_PendingRequests.Count() - 1; i >= 0; i-- )
	{
		if ( m_PendingRequests[i].pTexture == pTexture )
		{
			m_PendingRequests.Remove( i );
		}
	}
	
	// Remove from loading list
	m_LoadingTextures.FindAndRemove( pTexture );
}

void CTextureStreamingManager::OnTextureMemoryReleased( ITextureInternal* pTexture )
{
	AUTO_LOCK( m_Mutex );
	
	StreamingTextureInfo_t* pInfo = FindTextureInfo( pTexture );
	if ( pInfo )
	{
		m_nCurrentMemoryBytes -= pInfo->nCurrentSizeBytes;
		pInfo->residenceState = TEXTURE_RESIDENCE_NONE;
		pInfo->nCurrentSizeBytes = 0;
		pInfo->nCurrentMipLevel = 99;
	}
}

//-----------------------------------------------------------------------------
// Internal helpers
//-----------------------------------------------------------------------------
int CTextureStreamingManager::CalculateResidentMipLevel( int nWidth, int nHeight )
{
	int nMip = 0;
	int nTargetSize = 1 << m_Config.nResidentMipLevels; // e.g., 4 mips = 16x16 target
	
	// Find mip level where size <= nTargetSize
	while ( nWidth > nTargetSize || nHeight > nTargetSize )
	{
		nWidth >>= 1;
		nHeight >>= 1;
		nMip++;
	}
	
	return nMip;
}

int CTextureStreamingManager::CalculateTextureSizeBytes( int nWidth, int nHeight, int nMipCount, ImageFormat fmt )
{
	int nTotalBytes = 0;
	
	// Determine bytes per pixel or block size
	int nBytesPerBlock = 0;
	int nBlockSize = 1;
	
	switch ( fmt )
	{
		case IMAGE_FORMAT_DXT1:
			nBytesPerBlock = 8;
			nBlockSize = 4;
			break;
		case IMAGE_FORMAT_DXT3:
		case IMAGE_FORMAT_DXT5:
			nBytesPerBlock = 16;
			nBlockSize = 4;
			break;
		case IMAGE_FORMAT_RGBA8888:
		case IMAGE_FORMAT_BGRA8888:
		case IMAGE_FORMAT_ABGR8888:
		case IMAGE_FORMAT_ARGB8888:
			nBytesPerBlock = 4;
			break;
		case IMAGE_FORMAT_RGB888:
		case IMAGE_FORMAT_BGR888:
			nBytesPerBlock = 3;
			break;
		case IMAGE_FORMAT_RGB565:
		case IMAGE_FORMAT_BGR565:
		case IMAGE_FORMAT_BGRA5551:
		case IMAGE_FORMAT_BGRA4444:
			nBytesPerBlock = 2;
			break;
		case IMAGE_FORMAT_I8:
		case IMAGE_FORMAT_A8:
			nBytesPerBlock = 1;
			break;
		default:
			nBytesPerBlock = 4; // Default assumption
			break;
	}
	
	for ( int mip = 0; mip < nMipCount; mip++ )
	{
		int mipWidth = MAX( 1, nWidth >> mip );
		int mipHeight = MAX( 1, nHeight >> mip );
		
		if ( nBlockSize > 1 )
		{
			// Block-compressed: round up to block size
			int nBlocksX = ( mipWidth + nBlockSize - 1 ) / nBlockSize;
			int nBlocksY = ( mipHeight + nBlockSize - 1 ) / nBlockSize;
			nTotalBytes += nBlocksX * nBlocksY * nBytesPerBlock;
		}
		else
		{
			nTotalBytes += mipWidth * mipHeight * nBytesPerBlock;
		}
	}
	
	return nTotalBytes;
}

void CTextureStreamingManager::QueueLoadRequest( ITextureInternal* pTexture, int nTargetMip, bool bHighPriority )
{
	TextureStreamRequest_t request;
	request.pTexture = pTexture;
	request.nTargetMipLevel = nTargetMip;
	request.nPriority = bHighPriority ? 10 : 0;
	request.nRequestFrame = m_nCurrentFrame;
	
	m_PendingRequests.AddToTail( request );
}

StreamingTextureInfo_t* CTextureStreamingManager::FindTextureInfo( ITextureInternal* pTexture )
{
	int idx = m_StreamingTextures.Find( pTexture );
	if ( idx == m_StreamingTextures.InvalidIndex() )
		return NULL;
	
	return &m_StreamingTextures[idx];
}

StreamingTextureInfo_t* CTextureStreamingManager::GetOrCreateTextureInfo( ITextureInternal* pTexture )
{
	int idx = m_StreamingTextures.Find( pTexture );
	if ( idx == m_StreamingTextures.InvalidIndex() )
	{
		StreamingTextureInfo_t info;
		info.pTexture = pTexture;
		idx = m_StreamingTextures.Insert( pTexture, info );
	}
	
	return &m_StreamingTextures[idx];
}

//-----------------------------------------------------------------------------
// Debug
//-----------------------------------------------------------------------------
void CTextureStreamingManager::DebugPrintStatus()
{
	AUTO_LOCK( m_Mutex );
	
	Msg( "=== Texture Streaming Status ===\n" );
	Msg( "Enabled: %s\n", m_Config.bEnabled ? "yes" : "no" );
	Msg( "Memory: %d / %d MB\n", GetCurrentMemoryUsageMB(), m_Config.nStreamingBudgetMB );
	Msg( "Tracked textures: %d\n", m_StreamingTextures.Count() );
	Msg( "Pending loads: %d\n", m_PendingRequests.Count() );
	Msg( "Currently loading: %d\n", m_LoadingTextures.Count() );
	Msg( "Worker threads: %d\n", m_WorkerThreads.Count() );
	
	int nFull = 0, nPartial = 0, nLoading = 0;
	FOR_EACH_MAP_FAST( m_StreamingTextures, i )
	{
		switch ( m_StreamingTextures[i].residenceState )
		{
			case TEXTURE_RESIDENCE_FULL: nFull++; break;
			case TEXTURE_RESIDENCE_PARTIAL: nPartial++; break;
			case TEXTURE_RESIDENCE_LOADING: nLoading++; break;
			default: break;
		}
	}
	
	Msg( "Full: %d, Partial: %d, Loading: %d\n", nFull, nPartial, nLoading );
}

void CTextureStreamingManager::EnableStreaming( bool bEnable )
{
	AUTO_LOCK( m_Mutex );
	
	if ( m_Config.bEnabled == bEnable )
		return;
	
	m_Config.bEnabled = bEnable;
	mat_streaming_enabled.SetValue( bEnable ? 1 : 0 );
	
	if ( bEnable && m_WorkerThreads.Count() == 0 )
	{
		StartWorkerThreads();
	}
	else if ( !bEnable && m_WorkerThreads.Count() > 0 )
	{
		StopWorkerThreads();
	}
}

//-----------------------------------------------------------------------------
// Debug command - call via mat_streaming_status in console
// Note: Register this command in CMaterialSystem if needed
//-----------------------------------------------------------------------------
void Mat_StreamingStatus_f()
{
	TextureStreaming()->DebugPrintStatus();
}
