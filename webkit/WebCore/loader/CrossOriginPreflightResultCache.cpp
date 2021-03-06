

#include "config.h"
#include "CrossOriginPreflightResultCache.h"

#include "CrossOriginAccessControl.h"
#include "ResourceResponse.h"
#include <wtf/CurrentTime.h>
#include <wtf/StdLibExtras.h>
#include <wtf/Threading.h>

namespace WebCore {

// These values are at the discretion of the user agent.
static const unsigned defaultPreflightCacheTimeoutSeconds = 5;
static const unsigned maxPreflightCacheTimeoutSeconds = 600; // Should be short enough to minimize the risk of using a poisoned cache after switching to a secure network.

static bool parseAccessControlMaxAge(const String& string, unsigned& expiryDelta)
{
    // FIXME: this will not do the correct thing for a number starting with a '+'
    bool ok = false;
    expiryDelta = string.toUIntStrict(&ok);
    return ok;
}

template<class HashType>
static void addToAccessControlAllowList(const String& string, unsigned start, unsigned end, HashSet<String, HashType>& set)
{
    StringImpl* stringImpl = string.impl();
    if (!stringImpl)
        return;

    // Skip white space from start.
    while (start <= end && isSpaceOrNewline((*stringImpl)[start]))
        ++start;

    // only white space
    if (start > end) 
        return;

    // Skip white space from end.
    while (end && isSpaceOrNewline((*stringImpl)[end]))
        --end;

    set.add(string.substring(start, end - start + 1));
}

template<class HashType>
static bool parseAccessControlAllowList(const String& string, HashSet<String, HashType>& set)
{
    int start = 0;
    int end;
    while ((end = string.find(',', start)) != -1) {
        if (start == end)
            return false;

        addToAccessControlAllowList(string, start, end - 1, set);
        start = end + 1;
    }
    if (start != static_cast<int>(string.length()))
        addToAccessControlAllowList(string, start, string.length() - 1, set);

    return true;
}

bool CrossOriginPreflightResultCacheItem::parse(const ResourceResponse& response)
{
    m_methods.clear();
    if (!parseAccessControlAllowList(response.httpHeaderField("Access-Control-Allow-Methods"), m_methods))
        return false;

    m_headers.clear();
    if (!parseAccessControlAllowList(response.httpHeaderField("Access-Control-Allow-Headers"), m_headers))
        return false;

    unsigned expiryDelta;
    if (parseAccessControlMaxAge(response.httpHeaderField("Access-Control-Max-Age"), expiryDelta)) {
        if (expiryDelta > maxPreflightCacheTimeoutSeconds)
            expiryDelta = maxPreflightCacheTimeoutSeconds;
    } else
        expiryDelta = defaultPreflightCacheTimeoutSeconds;

    m_absoluteExpiryTime = currentTime() + expiryDelta;
    return true;
}

bool CrossOriginPreflightResultCacheItem::allowsCrossOriginMethod(const String& method) const
{
    return m_methods.contains(method) || isOnAccessControlSimpleRequestMethodWhitelist(method);
}

bool CrossOriginPreflightResultCacheItem::allowsCrossOriginHeaders(const HTTPHeaderMap& requestHeaders) const
{
    HTTPHeaderMap::const_iterator end = requestHeaders.end();
    for (HTTPHeaderMap::const_iterator it = requestHeaders.begin(); it != end; ++it) {
        if (!m_headers.contains(it->first) && !isOnAccessControlSimpleRequestHeaderWhitelist(it->first, it->second))
            return false;
    }
    return true;
}

bool CrossOriginPreflightResultCacheItem::allowsRequest(bool includeCredentials, const String& method, const HTTPHeaderMap& requestHeaders) const
{
    if (m_absoluteExpiryTime < currentTime())
        return false;
    if (includeCredentials && !m_credentials)
        return false;
    if (!allowsCrossOriginMethod(method))
        return false;
    if (!allowsCrossOriginHeaders(requestHeaders))
        return false;
    return true;
}

CrossOriginPreflightResultCache& CrossOriginPreflightResultCache::shared()
{
    DEFINE_STATIC_LOCAL(CrossOriginPreflightResultCache, cache, ());
    ASSERT(isMainThread());
    return cache;
}

void CrossOriginPreflightResultCache::appendEntry(const String& origin, const KURL& url, CrossOriginPreflightResultCacheItem* preflightResult)
{
    ASSERT(isMainThread());
    m_preflightHashMap.set(std::make_pair(origin, url), preflightResult);
}

bool CrossOriginPreflightResultCache::canSkipPreflight(const String& origin, const KURL& url, bool includeCredentials, const String& method, const HTTPHeaderMap& requestHeaders)
{
    ASSERT(isMainThread());
    CrossOriginPreflightResultHashMap::iterator cacheIt = m_preflightHashMap.find(std::make_pair(origin, url));
    if (cacheIt == m_preflightHashMap.end())
        return false;

    if (cacheIt->second->allowsRequest(includeCredentials, method, requestHeaders))
        return true;

    delete cacheIt->second;
    m_preflightHashMap.remove(cacheIt);
    return false;
}

void CrossOriginPreflightResultCache::empty()
{
    ASSERT(isMainThread());
    deleteAllValues(m_preflightHashMap);
    m_preflightHashMap.clear();
}

} // namespace WebCore
