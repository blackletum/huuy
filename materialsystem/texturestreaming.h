//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Moderate texture streaming system
// Maintains full compatibility with existing FindOrLoadTexture API
//
//=============================================================================

#ifndef TEXTURESTREAMING_H
#define TEXTURESTREAMING_H

#ifdef _WIN32
#pragma once
#endif

#include "tier1/utlvector.h"
#include "tier1/utlmap.h"
#include "tier1/utlqueue.h"
#include "tier0/tslist.h"
#include "tier0/threadtools.h"
#include "itextureinternal.h"

// Forward declarations
class IVTFTexture;
class ITextureInternal;

//-----------------------------------------------------------------------------
// Streaming configuration - Moderate mode
//-----------------------------------------------------------------------------
struct TextureStreamingConfig_t
{
	int		nResidentMipLevels;			// Mip levels always in memory (0 = highest res)
	int		nMaxConcurrentLoads;		// Max parallel loading operations
	int		nStreamingBudgetMB;			// Memory budget for streamed textures
	int		nEvictionDelayFrames;		// Frames before evicting unused textures
	int		nMinTextureSize;			// Minimum texture size for streaming (smaller always loaded)
	bool	bEnabled;					// Is streaming enabled
	
	TextureStreamingConfig_t()
		: nResidentMipLevels( 4 )		// 128x128 and smaller always resident
		, nMaxConcurrentLoads( 8 )
		, nStreamingBudgetMB( 200 )
		, nEvictionDelayFrames( 300 )	// ~5 seconds at 60fps
		, nMinTextureSize( 64 )			// Textures <= 64x64 always fully loaded
		, bEnabled( true )
	{}
};

//-----------------------------------------------------------------------------
// Texture residence state
//-----------------------------------------------------------------------------
enum TextureResidenceState_t
{
	TEXTURE_RESIDENCE_NONE = 0,		// Not loaded at all
	TEXTURE_RESIDENCE_PARTIAL,		// Only low mips loaded (resident mips)
	TEXTURE_RESIDENCE_FULL,			// All mips loaded
	TEXTURE_RESIDENCE_LOADING,		// Currently loading high mips
};

//-----------------------------------------------------------------------------
// Streaming request for a texture
//-----------------------------------------------------------------------------
struct TextureStreamRequest_t
{
	ITextureInternal*	pTexture;
	int					nTargetMipLevel;	// 0 = full resolution
	int					nPriority;			// Higher = more important
	uint32				nRequestFrame;		// Frame when requested
	
	TextureStreamRequest_t()
		: pTexture( NULL )
		, nTargetMipLevel( 0 )
		, nPriority( 0 )
		, nRequestFrame( 0 )
	{}
};

//-----------------------------------------------------------------------------
// Info tracked per streaming texture
//-----------------------------------------------------------------------------
struct StreamingTextureInfo_t
{
	ITextureInternal*			pTexture;
	TextureResidenceState_t		residenceState;
	int							nCurrentMipLevel;	// Current lowest loaded mip (0 = full)
	int							nFullSizeBytes;		// Size when fully loaded
	int							nCurrentSizeBytes;	// Current memory usage
	uint32						nLastUsedFrame;		// Frame when last rendered
	uint32						nLoadStartFrame;	// Frame when loading started
	bool						bPendingEviction;	// Marked for eviction
	
	StreamingTextureInfo_t()
		: pTexture( NULL )
		, residenceState( TEXTURE_RESIDENCE_NONE )
		, nCurrentMipLevel( 99 )
		, nFullSizeBytes( 0 )
		, nCurrentSizeBytes( 0 )
		, nLastUsedFrame( 0 )
		, nLoadStartFrame( 0 )
		, bPendingEviction( false )
	{}
};

//-----------------------------------------------------------------------------
// Async load job
//-----------------------------------------------------------------------------
struct StreamingLoadJob_t
{
	ITextureInternal*	pTexture;
	IVTFTexture*		pScratchVTF;
	int					nTargetMipLevel;
	int					nAdditionalFlags;
	bool				bHighPriority;
	
	StreamingLoadJob_t()
		: pTexture( NULL )
		, pScratchVTF( NULL )
		, nTargetMipLevel( 0 )
		, nAdditionalFlags( 0 )
		, bHighPriority( false )
	{}
};

//-----------------------------------------------------------------------------
// Main texture streaming manager
//-----------------------------------------------------------------------------
class CTextureStreamingManager
{
public:
	CTextureStreamingManager();
	~CTextureStreamingManager();
	
	// Initialization
	void Init( const TextureStreamingConfig_t* pConfig = NULL );
	void Shutdown();
	
	// Configuration
	void SetConfig( const TextureStreamingConfig_t& config );
	const TextureStreamingConfig_t& GetConfig() const { return m_Config; }
	
	// Main update - call once per frame from main thread
	void Update();
	
	//-------------------------------------------------------------------------
	// Texture loading interface - maintains compatibility with existing code
	//-------------------------------------------------------------------------
	
	// Called by TextureManager when a texture is about to be loaded
	// Returns true if streaming should handle this texture
	bool ShouldStreamTexture( ITextureInternal* pTexture, int nWidth, int nHeight, int nFlags );
	
	// Called instead of full Download() for streaming textures
	// Loads only resident mip levels initially
	bool LoadTexturePartial( ITextureInternal* pTexture, int nAdditionalFlags );
	
	// Request full resolution for a texture (call when texture is about to be rendered)
	void RequestFullResolution( ITextureInternal* pTexture );
	
	// Force immediate full load (for critical textures)
	// This blocks until complete - use sparingly
	void ForceFullLoad( ITextureInternal* pTexture );
	
	// Mark texture as no longer needed at full resolution
	void ReleaseFullResolution( ITextureInternal* pTexture );
	
	//-------------------------------------------------------------------------
	// Query interface
	//-------------------------------------------------------------------------
	
	// Get current residence state
	TextureResidenceState_t GetTextureResidence( ITextureInternal* pTexture ) const;
	
	// Check if texture is at full resolution
	bool IsTextureFullyLoaded( ITextureInternal* pTexture ) const;
	
	// Get current memory usage
	int GetCurrentMemoryUsageMB() const;
	int GetStreamingBudgetMB() const { return m_Config.nStreamingBudgetMB; }
	
	// Statistics
	int GetTexturesStreaming() const { return m_LoadingTextures.Count(); }
	int GetTexturesPendingLoad() const { return m_PendingRequests.Count(); }
	int GetTotalStreamedTextures() const { return m_StreamingTextures.Count(); }
	
	//-------------------------------------------------------------------------
	// Texture lifecycle callbacks - called by TextureManager
	//-------------------------------------------------------------------------
	
	// Called when texture is being destroyed
	void OnTextureDestroyed( ITextureInternal* pTexture );
	
	// Called when texture memory is released (e.g., device lost)
	void OnTextureMemoryReleased( ITextureInternal* pTexture );
	
	//-------------------------------------------------------------------------
	// Debug
	//-------------------------------------------------------------------------
	void DebugPrintStatus();
	void EnableStreaming( bool bEnable );
	bool IsStreamingEnabled() const { return m_Config.bEnabled; }

private:
	// Worker thread functions
	void StartWorkerThreads();
	void StopWorkerThreads();
	static uintp LoaderThreadMain( void* pParam );
	void LoaderThread_Main();
	void LoaderThread_ProcessJob( StreamingLoadJob_t* pJob );
	
	// Internal helpers
	bool IsTextureEligibleForStreaming( ITextureInternal* pTexture, int nWidth, int nHeight, int nFlags );
	int CalculateResidentMipLevel( int nWidth, int nHeight );
	int CalculateTextureSizeBytes( int nWidth, int nHeight, int nMipCount, ImageFormat fmt );
	void QueueLoadRequest( ITextureInternal* pTexture, int nTargetMip, bool bHighPriority );
	void ProcessCompletedLoads();
	void UpdateEvictions();
	void EvictTexture( ITextureInternal* pTexture );
	StreamingTextureInfo_t* FindTextureInfo( ITextureInternal* pTexture );
	StreamingTextureInfo_t* GetOrCreateTextureInfo( ITextureInternal* pTexture );
	
	// Configuration
	TextureStreamingConfig_t m_Config;
	
	// Texture tracking
	CUtlMap<ITextureInternal*, StreamingTextureInfo_t> m_StreamingTextures;
	
	// Request queues
	CUtlVector<TextureStreamRequest_t> m_PendingRequests;
	CUtlVector<ITextureInternal*> m_LoadingTextures;
	
	// Thread-safe queues for async loading
	CTSQueue<StreamingLoadJob_t*> m_JobQueue;
	CTSQueue<StreamingLoadJob_t*> m_CompletedJobs;
	CTSQueue<IVTFTexture*> m_ScratchVTFPool;
	
	// Worker threads
	CUtlVector<ThreadHandle_t> m_WorkerThreads;
	volatile bool m_bShutdown;
	
	// Memory tracking
	CInterlockedInt m_nCurrentMemoryBytes;
	
	// Frame counter
	uint32 m_nCurrentFrame;
	
	// Thread safety
	mutable CThreadFastMutex m_Mutex;
	
	// Singleton pattern support
	static CTextureStreamingManager* s_pInstance;
	
public:
	static CTextureStreamingManager* GetInstance();
};

//-----------------------------------------------------------------------------
// Global accessor
//-----------------------------------------------------------------------------
inline CTextureStreamingManager* TextureStreaming()
{
	return CTextureStreamingManager::GetInstance();
}

#endif // TEXTURESTREAMING_H
