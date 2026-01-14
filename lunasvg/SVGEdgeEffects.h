//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Edge blur and tint effects for SVG bitmaps
//
//=============================================================================//

#ifndef SVGEDGEEFFECTS_H
#define SVGEDGEEFFECTS_H

#include <cstdint>
#include <vector>
#include <cmath>
#include <algorithm>

namespace SVGEffects
{

//-----------------------------------------------------------------------------
// Простое размытие краёв (обрабатывает только альфа-канал по краям)
//-----------------------------------------------------------------------------
void BlurEdges(uint8_t* data, int width, int height, int blurRadius = 5)
{
    if (!data || width <= 0 || height <= 0 || blurRadius <= 0)
        return;

    std::vector<uint8_t> tempAlpha(width * height);
    
    // Извлекаем альфа канал
    for (int i = 0; i < width * height; i++)
    {
        tempAlpha[i] = data[i * 4 + 3]; // RGBA, берём A
    }
    
    // Применяем box blur к альфа каналу (быстрая аппроксимация gaussian blur)
    std::vector<uint8_t> blurred = tempAlpha;
    
    // Горизонтальный проход
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            int sum = 0;
            int count = 0;
            
            for (int dx = -blurRadius; dx <= blurRadius; dx++)
            {
                int sx = x + dx;
                if (sx >= 0 && sx < width)
                {
                    sum += tempAlpha[y * width + sx];
                    count++;
                }
            }
            
            blurred[y * width + x] = count > 0 ? sum / count : 0;
        }
    }
    
    // Вертикальный проход
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            int sum = 0;
            int count = 0;
            
            for (int dy = -blurRadius; dy <= blurRadius; dy++)
            {
                int sy = y + dy;
                if (sy >= 0 && sy < height)
                {
                    sum += blurred[sy * width + x];
                    count++;
                }
            }
            
            tempAlpha[y * width + x] = count > 0 ? sum / count : 0;
        }
    }
    
    // Записываем размытую альфу обратно
    for (int i = 0; i < width * height; i++)
    {
        data[i * 4 + 3] = tempAlpha[i];
    }
}

//-----------------------------------------------------------------------------
// Окраска краёв (добавляет цветное свечение по краям)
//-----------------------------------------------------------------------------
void TintEdges(uint8_t* data, int width, int height, uint8_t r, uint8_t g, uint8_t b, int edgeWidth = 5)
{
    if (!data || width <= 0 || height <= 0 || edgeWidth <= 0)
        return;

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            int idx = (y * width + x) * 4;
            uint8_t alpha = data[idx + 3];
            
            // Проверяем, находится ли пиксель рядом с краем (альфа-граница)
            bool isEdge = false;
            
            // Проверяем соседние пиксели
            for (int dy = -edgeWidth; dy <= edgeWidth && !isEdge; dy++)
            {
                for (int dx = -edgeWidth; dx <= edgeWidth && !isEdge; dx++)
                {
                    int nx = x + dx;
                    int ny = y + dy;
                    
                    if (nx >= 0 && nx < width && ny >= 0 && ny < height)
                    {
                        int nidx = (ny * width + nx) * 4;
                        uint8_t nAlpha = data[nidx + 3];
                        
                        // Если есть переход от прозрачного к непрозрачному - это край
                        if ((alpha > 128 && nAlpha < 128) || (alpha < 128 && nAlpha > 128))
                        {
                            isEdge = true;
                        }
                    }
                }
            }
            
            // Если это край, смешиваем с цветом окраски
            if (isEdge && alpha > 10)
            {
                float blend = 0.5f; // Сила окраски
                data[idx + 0] = (uint8_t)(data[idx + 0] * (1.0f - blend) + r * blend);
                data[idx + 1] = (uint8_t)(data[idx + 1] * (1.0f - blend) + g * blend);
                data[idx + 2] = (uint8_t)(data[idx + 2] * (1.0f - blend) + b * blend);
            }
        }
    }
}

//-----------------------------------------------------------------------------
// Комбинированный эффект: размытие + окраска краёв
//-----------------------------------------------------------------------------
void BlurAndTintEdges(uint8_t* data, int width, int height, 
                      uint8_t r, uint8_t g, uint8_t b, 
                      int blurRadius = 5, int tintWidth = 3)
{
    // Сначала окрашиваем края
    TintEdges(data, width, height, r, g, b, tintWidth);
    
    // Потом размываем для плавного перехода
    BlurEdges(data, width, height, blurRadius);
}

//-----------------------------------------------------------------------------
// Добавление свечения (glow) по краям
//-----------------------------------------------------------------------------
void AddEdgeGlow(uint8_t* data, int width, int height, 
                 uint8_t r, uint8_t g, uint8_t b, uint8_t glowAlpha = 128, 
                 int glowRadius = 10)
{
    if (!data || width <= 0 || height <= 0)
        return;

    std::vector<uint8_t> glowMap(width * height, 0);
    
    // Находим края и создаём карту свечения
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            int idx = (y * width + x) * 4;
            uint8_t alpha = data[idx + 3];
            
            // Если пиксель на границе непрозрачности
            if (alpha > 10 && alpha < 245)
            {
                // Распространяем свечение вокруг
                for (int dy = -glowRadius; dy <= glowRadius; dy++)
                {
                    for (int dx = -glowRadius; dx <= glowRadius; dx++)
                    {
                        int nx = x + dx;
                        int ny = y + dy;
                        
                        if (nx >= 0 && nx < width && ny >= 0 && ny < height)
                        {
                            float dist = sqrtf(dx * dx + dy * dy);
                            if (dist <= glowRadius)
                            {
                                int gidx = ny * width + nx;
                                float intensity = 1.0f - (dist / glowRadius);
                                uint8_t glowValue = (uint8_t)(glowAlpha * intensity);
                                glowMap[gidx] = std::max(glowMap[gidx], glowValue);
                            }
                        }
                    }
                }
            }
        }
    }
    
    // Применяем свечение
    for (int i = 0; i < width * height; i++)
    {
        if (glowMap[i] > 0)
        {
            int idx = i * 4;
            uint8_t currentAlpha = data[idx + 3];
            
            // Добавляем свечение только там, где альфа низкая (вокруг объекта)
            if (currentAlpha < 50)
            {
                float blend = glowMap[i] / 255.0f;
                data[idx + 0] = (uint8_t)(data[idx + 0] * (1.0f - blend) + r * blend);
                data[idx + 1] = (uint8_t)(data[idx + 1] * (1.0f - blend) + g * blend);
                data[idx + 2] = (uint8_t)(data[idx + 2] * (1.0f - blend) + b * blend);
                data[idx + 3] = std::max(currentAlpha, glowMap[i]);
            }
        }
    }
}

} // namespace SVGEffects

#endif // SVGEDGEEFFECTS_H
