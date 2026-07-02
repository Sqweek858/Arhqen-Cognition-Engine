#pragma once

#include "ArhqenCognitionEngine/Ui/D2D/DWriteFontEngine.h"

#include <cstdint>
#include <list>
#include <unordered_map>

namespace am::ui
{
    struct TextCacheKey
    {
        std::wstring text;
        FontRole role = FontRole::Body;
        float width = 0.0f;
        float height = 0.0f;
        DWRITE_TEXT_ALIGNMENT horizontal = DWRITE_TEXT_ALIGNMENT_LEADING;
        DWRITE_PARAGRAPH_ALIGNMENT vertical = DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
        DWRITE_WORD_WRAPPING wrapping = DWRITE_WORD_WRAPPING_WRAP;
        bool trimEnd = false;
        std::uint64_t fontGeneration = 0;

        bool operator==(const TextCacheKey& other) const;
    };

    struct TextCacheKeyHasher
    {
        std::size_t operator()(const TextCacheKey& key) const;
    };

    class DWriteTextCache
    {
    public:
        explicit DWriteTextCache(std::size_t capacity = 256);

        void setCapacity(std::size_t capacity);
        void clear();
        std::size_t size() const;
        std::size_t capacity() const;

        TextLayoutResult getOrCreate(const DWriteFontEngine& engine, const std::wstring& text, const TextLayoutOptions& options);
        std::size_t hitCount() const;
        std::size_t missCount() const;

    private:
        struct Entry
        {
            TextCacheKey key;
            TextLayoutResult layout;
        };

        TextCacheKey makeKey(const DWriteFontEngine& engine, const std::wstring& text, const TextLayoutOptions& options) const;
        void touch(std::list<Entry>::iterator it);
        void evictIfNeeded();

        std::size_t capacity_ = 256;
        std::list<Entry> entries_;
        std::unordered_map<TextCacheKey, std::list<Entry>::iterator, TextCacheKeyHasher> lookup_;
        std::size_t hits_ = 0;
        std::size_t misses_ = 0;
    };
}
