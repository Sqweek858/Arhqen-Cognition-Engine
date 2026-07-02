#include "ArhqenCognitionEngine/Ui/D2D/DWriteTextCache.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>

namespace am::ui
{
    namespace
    {
        std::size_t combineHash(std::size_t seed, std::size_t value)
        {
            return seed ^ (value + 0x9e3779b97f4a7c15ull + (seed << 6) + (seed >> 2));
        }

        int quantize(float value)
        {
            if (!std::isfinite(value))
            {
                return 0;
            }
            constexpr float kLimit = static_cast<float>(std::numeric_limits<int>::max() / 4);
            return static_cast<int>(std::round(std::clamp(value, -kLimit, kLimit) * 2.0f));
        }

        float normalizedDimension(float value)
        {
            return std::isfinite(value) ? std::max(1.0f, value) : 1.0f;
        }
    }

    bool TextCacheKey::operator==(const TextCacheKey& other) const
    {
        return text == other.text &&
            role == other.role &&
            quantize(width) == quantize(other.width) &&
            quantize(height) == quantize(other.height) &&
            horizontal == other.horizontal &&
            vertical == other.vertical &&
            wrapping == other.wrapping &&
            trimEnd == other.trimEnd &&
            fontGeneration == other.fontGeneration;
    }

    std::size_t TextCacheKeyHasher::operator()(const TextCacheKey& key) const
    {
        std::size_t seed = std::hash<std::wstring>{}(key.text);
        seed = combineHash(seed, std::hash<int>{}(static_cast<int>(key.role)));
        seed = combineHash(seed, std::hash<int>{}(quantize(key.width)));
        seed = combineHash(seed, std::hash<int>{}(quantize(key.height)));
        seed = combineHash(seed, std::hash<int>{}(static_cast<int>(key.horizontal)));
        seed = combineHash(seed, std::hash<int>{}(static_cast<int>(key.vertical)));
        seed = combineHash(seed, std::hash<int>{}(static_cast<int>(key.wrapping)));
        seed = combineHash(seed, std::hash<bool>{}(key.trimEnd));
        seed = combineHash(seed, std::hash<std::uint64_t>{}(key.fontGeneration));
        return seed;
    }

    DWriteTextCache::DWriteTextCache(std::size_t capacity)
        : capacity_(std::max<std::size_t>(1, capacity))
    {
    }

    void DWriteTextCache::setCapacity(std::size_t capacity)
    {
        capacity_ = std::max<std::size_t>(1, capacity);
        evictIfNeeded();
    }

    void DWriteTextCache::clear()
    {
        lookup_.clear();
        entries_.clear();
        hits_ = 0;
        misses_ = 0;
    }

    std::size_t DWriteTextCache::size() const
    {
        return entries_.size();
    }

    std::size_t DWriteTextCache::capacity() const
    {
        return capacity_;
    }

    TextLayoutResult DWriteTextCache::getOrCreate(const DWriteFontEngine& engine, const std::wstring& text, const TextLayoutOptions& options)
    {
        const auto key = makeKey(engine, text, options);
        auto found = lookup_.find(key);

        if (found != lookup_.end())
        {
            ++hits_;
            touch(found->second);
            return found->second->layout;
        }

        ++misses_;
        TextLayoutResult created = engine.createLayout(text, options);
        if (!created.valid || !created.layout)
        {
            return created;
        }

        Entry entry;
        entry.key = key;
        entry.layout = created;

        entries_.push_front(std::move(entry));
        lookup_[entries_.front().key] = entries_.begin();
        evictIfNeeded();

        return created;
    }

    std::size_t DWriteTextCache::hitCount() const
    {
        return hits_;
    }

    std::size_t DWriteTextCache::missCount() const
    {
        return misses_;
    }

    TextCacheKey DWriteTextCache::makeKey(const DWriteFontEngine& engine, const std::wstring& text, const TextLayoutOptions& options) const
    {
        TextCacheKey key;
        key.text = text;
        key.role = options.role;
        key.width = normalizedDimension(options.width);
        key.height = normalizedDimension(options.height);
        key.horizontal = options.horizontal;
        key.vertical = options.vertical;
        key.wrapping = options.wrapping;
        key.trimEnd = options.trimEnd;
        key.fontGeneration = engine.generation();
        return key;
    }

    void DWriteTextCache::touch(std::list<Entry>::iterator it)
    {
        if (it == entries_.begin())
        {
            return;
        }

        entries_.splice(entries_.begin(), entries_, it);
    }

    void DWriteTextCache::evictIfNeeded()
    {
        while (entries_.size() > capacity_)
        {
            lookup_.erase(entries_.back().key);
            entries_.pop_back();
        }
    }
}
