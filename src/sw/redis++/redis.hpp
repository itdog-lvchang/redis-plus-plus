/**************************************************************************
   Copyright (c) 2017 sewenew

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 *************************************************************************/

#ifndef SEWENEW_REDISPLUSPLUS_REDIS_HPP
#define SEWENEW_REDISPLUSPLUS_REDIS_HPP

#include "command.h"
#include "reply.h"
#include "utils.h"
#include "errors.h"

namespace sw {

namespace redis {

namespace detail {

template <typename Output>
long long parse_scan_reply(redisReply &reply, Output output) {
    if (reply.elements != 2 || reply.element == nullptr) {
        throw ProtoError("Invalid scan reply");
    }

    auto *cursor_reply = reply.element[0];
    auto *data_reply = reply.element[1];
    if (cursor_reply == nullptr || data_reply == nullptr) {
        throw ProtoError("Invalid cursor reply or data reply");
    }

    auto cursor_str = reply::parse<std::string>(*cursor_reply);
    auto new_cursor = 0;
    try {
        new_cursor = std::stoll(cursor_str);
    } catch (const std::exception &e) {
        throw ProtoError("Invalid cursor reply: " + cursor_str);
    }

    reply::to_array(*data_reply, output);

    return new_cursor;
}

}

template <typename Cmd, typename ...Args>
ReplyUPtr Redis::command(Cmd cmd, Args &&...args) {
    auto connection = _pool.fetch();

    assert(!connection.broken());

    ConnectionPoolGuard guard(_pool, connection);

    cmd(connection, std::forward<Args>(args)...);

    auto reply = connection.recv();

    return reply;
}

// KEY commands.

template <typename Input>
long long Redis::del(Input first, Input last) {
    auto reply = command(cmd::del_range<Input>, first, last);

    return reply::parse<long long>(*reply);
}

template <typename Input>
long long Redis::exists(Input first, Input last) {
    auto reply = command(cmd::exists_range<Input>, first, last);

    return reply::parse<long long>(*reply);
}

inline bool Redis::expire(const StringView &key, const std::chrono::seconds &timeout) {
    return expire(key, timeout.count());
}

inline bool Redis::expireat(const StringView &key,
                                    const std::chrono::time_point<std::chrono::system_clock,
                                                                    std::chrono::seconds> &tp) {
    return expireat(key, tp.time_since_epoch().count());
}

template <typename Output>
void Redis::keys(const StringView &pattern, Output output) {
    auto reply = command(cmd::keys, pattern);

    reply::to_array(*reply, output);
}

inline bool Redis::pexpire(const StringView &key, const std::chrono::milliseconds &timeout) {
    return expire(key, timeout.count());
}

inline bool Redis::pexpireat(const StringView &key,
                                const std::chrono::time_point<std::chrono::system_clock,
                                                                std::chrono::milliseconds> &tp) {
    return pexpireat(key, tp.time_since_epoch().count());
}

inline void Redis::restore(const StringView &key,
                            const std::chrono::milliseconds &ttl,
                            const StringView &val,
                            bool replace) {
    return restore(key, ttl.count(), val, replace);
}

template <typename Output>
long long Redis::scan(long long cursor,
                    const StringView &pattern,
                    long long count,
                    Output output) {
    auto reply = command(cmd::scan, cursor, pattern, count);

    return detail::parse_scan_reply(*reply, output);
}

template <typename Output>
inline long long Redis::scan(long long cursor,
                                const StringView &pattern,
                                Output output) {
    return scan(cursor, pattern, 10, output);
}

template <typename Output>
inline long long Redis::scan(long long cursor,
                                long long count,
                                Output output) {
    return scan(cursor, "*", count, output);
}

template <typename Output>
inline long long Redis::scan(long long cursor,
                                Output output) {
    return scan(cursor, "*", 10, output);
}

template <typename Input>
long long Redis::touch(Input first, Input last) {
    auto reply = command(cmd::touch_range<Input>, first, last);

    return reply::parse<long long>(*reply);
}

template <typename Input>
long long Redis::unlink(Input first, Input last) {
    auto reply = command(cmd::unlink_range, first, last);

    return reply::parse<long long>(*reply);
}

inline long long Redis::wait(long long numslaves, const std::chrono::milliseconds &timeout) {
    return wait(numslaves, timeout.count());
}

// STRING commands.

template <typename Input>
long long Redis::bitop(BitOp op, const StringView &destination, Input first, Input last) {
    auto reply = command(cmd::bitop<Input>, op, destination, first, last);

    return reply::parse<long long>(*reply);
}

template <typename Input, typename Output>
void Redis::mget(Input first, Input last, Output output) {
    auto reply = command(cmd::mget<Input>, first, last);

    reply::to_array(*reply, output);
}

template <typename Input>
void Redis::mset(Input first, Input last) {
    auto reply = command(cmd::mset<Input>, first, last);

    reply::expect_ok_status(*reply);
}

template <typename Input>
bool Redis::msetnx(Input first, Input last) {
    auto reply = command(cmd::msetnx<Input>, first, last);

    return reply::parse<bool>(*reply);
}

inline void Redis::psetex(const StringView &key,
                            const std::chrono::milliseconds &ttl,
                            const StringView &val) {
    return psetex(key, ttl.count(), val);
}

inline void Redis::setex(const StringView &key,
                            const std::chrono::seconds &ttl,
                            const StringView &val) {
    setex(key, ttl.count(), val);
}

// LIST commands.

template <typename Input>
OptionalStringPair Redis::blpop(Input first, Input last, long long timeout) {
    auto reply = command(cmd::blpop<Input>, first, last, timeout);

    return reply::parse<OptionalStringPair>(*reply);
}

template <typename Input>
OptionalStringPair Redis::blpop(Input first,
                                Input last,
                                const std::chrono::seconds &timeout) {
    return blpop(first, last, timeout.count());
}

template <typename Input>
OptionalStringPair Redis::brpop(Input first, Input last, long long timeout) {
    auto reply = command(cmd::brpop<Input>, first, last, timeout);

    return reply::parse<OptionalStringPair>(*reply);
}

template <typename Input>
OptionalStringPair Redis::brpop(Input first,
                                Input last,
                                const std::chrono::seconds &timeout) {
    return brpop(first, last, timeout.count());
}

inline OptionalString Redis::brpoplpush(const StringView &source,
                                        const StringView &destination,
                                        const std::chrono::seconds &timeout) {
    return brpoplpush(source, destination, timeout.count());
}

template <typename Input>
inline long long Redis::lpush(const StringView &key, Input first, Input last) {
    auto reply = command(cmd::lpush_range<Input>, key, first, last);

    return reply::parse<long long>(*reply);
}

template <typename Output>
inline void Redis::lrange(const StringView &key, long long start, long long stop, Output output) {
    auto reply = command(cmd::lrange, key, start, stop);

    reply::to_array(*reply, output);
}

template <typename Input>
inline long long Redis::rpush(const StringView &key, Input first, Input last) {
    auto reply = command(cmd::rpush_range<Input>, key, first, last);

    return reply::parse<long long>(*reply);
}

// HASH commands.

template <typename Input>
inline long long Redis::hdel(const StringView &key, Input first, Input last) {
    auto reply = command(cmd::hdel_range<Input>, key, first, last);

    return reply::parse<long long>(*reply);
}

template <typename Output>
inline void Redis::hgetall(const StringView &key, Output output) {
    auto reply = command(cmd::hgetall, key);

    reply::to_array(*reply, output);
}

template <typename Output>
inline void Redis::hkeys(const StringView &key, Output output) {
    auto reply = command(cmd::hkeys, key);

    reply::to_array(*reply, output);
}

template <typename Input, typename Output>
inline void Redis::hmget(const StringView &key, Input first, Input last, Output output) {
    auto reply = command(cmd::hmget<Input>, key, first, last);

    reply::to_array(*reply, output);
}

template <typename Input>
inline void Redis::hmset(const StringView &key, Input first, Input last) {
    auto reply = command(cmd::hmset<Input>, key, first, last);

    reply::expect_ok_status(*reply);
}

template <typename Output>
long long Redis::hscan(const StringView &key,
                        long long cursor,
                        const StringView &pattern,
                        long long count,
                        Output output) {
    auto reply = command(cmd::hscan, key, cursor, pattern, count);

    return detail::parse_scan_reply(*reply, output);
}

template <typename Output>
inline long long Redis::hscan(const StringView &key,
                                long long cursor,
                                const StringView &pattern,
                                Output output) {
    return hscan(key, cursor, pattern, 10, output);
}

template <typename Output>
inline long long Redis::hscan(const StringView &key,
                                long long cursor,
                                long long count,
                                Output output) {
    return hscan(key, cursor, "*", count, output);
}

template <typename Output>
inline long long Redis::hscan(const StringView &key,
                                long long cursor,
                                Output output) {
    return hscan(key, cursor, "*", 10, output);
}

template <typename Output>
inline void Redis::hvals(const StringView &key, Output output) {
    auto reply = command(cmd::hvals, key);

    reply::to_array(*reply, output);
}

// SET commands.

template <typename Input>
long long Redis::sadd(const StringView &key, Input first, Input last) {
    auto reply = command(cmd::sadd_range<Input>, key, first, last);

    return reply::parse<long long>(*reply);
}

template <typename Input, typename Output>
void Redis::sdiff(Input first, Input last, Output output) {
    auto reply = command(cmd::sdiff<Input>, first, last);

    reply::to_array(*reply, output);
}

template <typename Input>
long long Redis::sdiffstore(const StringView &destination,
                            Input first,
                            Input last) {
    auto reply = command(cmd::sdiffstore<Input>, destination, first, last);

    return reply::parse<long long>(*reply);
}

template <typename Input, typename Output>
void Redis::sinter(Input first, Input last, Output output) {
    auto reply = command(cmd::sinter<Input>, first, last);

    reply::to_array(*reply, output);
}

template <typename Input>
long long Redis::sinterstore(const StringView &destination,
                            Input first,
                            Input last) {
    auto reply = command(cmd::sinterstore<Input>, destination, first, last);

    return reply::parse<long long>(*reply);
}

template <typename Output>
void Redis::smembers(const StringView &key, Output output) {
    auto reply = command(cmd::smembers, key);

    reply::to_array(*reply, output);
}

template <typename Output>
void Redis::spop(const StringView &key, long long count, Output output) {
    auto reply = command(cmd::spop_range, key, count);

    reply::to_array(*reply, output);
}

template <typename Output>
void Redis::srandmember(const StringView &key, long long count, Output output) {
    auto reply = command(cmd::srandmember_range, key, count);

    reply::to_array(*reply, output);
}

template <typename Input>
long long Redis::srem(const StringView &key, Input first, Input last) {
    auto reply = command(cmd::srem_range<Input>, key, first, last);

    return reply::parse<long long>(*reply);
}

template <typename Output>
long long Redis::sscan(const StringView &key,
                        long long cursor,
                        const StringView &pattern,
                        long long count,
                        Output output) {
    auto reply = command(cmd::sscan, key, cursor, pattern, count);

    return detail::parse_scan_reply(*reply, output);
}

template <typename Output>
inline long long Redis::sscan(const StringView &key,
                                long long cursor,
                                const StringView &pattern,
                                Output output) {
    return sscan(key, cursor, pattern, 10, output);
}

template <typename Output>
inline long long Redis::sscan(const StringView &key,
                                long long cursor,
                                long long count,
                                Output output) {
    return sscan(key, cursor, "*", count, output);
}

template <typename Output>
inline long long Redis::sscan(const StringView &key,
                                long long cursor,
                                Output output) {
    return sscan(key, cursor, "*", 10, output);
}

template <typename Input, typename Output>
void Redis::sunion(Input first, Input last, Output output) {
    auto reply = command(cmd::sunion<Input>, first, last);

    reply::to_array(*reply, output);
}

template <typename Input>
long long Redis::sunionstore(const StringView &destination, Input first, Input last) {
    auto reply = command(cmd::sunionstore<Input>, destination, first, last);

    return reply::parse<long long>(*reply);
}

// SORTED SET commands.

template <typename Input>
long long Redis::zadd(const StringView &key,
                        Input first,
                        Input last,
                        bool changed,
                        UpdateType type) {
    auto reply = command(cmd::zadd_range<Input>, key, first, last, changed, type);

    return reply::parse<long long>(*reply);
}

template <typename Interval>
long long Redis::zcount(const StringView &key, const Interval &interval) {
    auto reply = command(cmd::zcount<Interval>, key, interval);

    return reply::parse<long long>(*reply);
}

template <typename Input>
long long Redis::zinterstore(const StringView &destination,
                                Input first,
                                Input last,
                                Aggregation type) {
    auto reply = command(cmd::zinterstore<Input>,
                            destination,
                            first,
                            last,
                            type);

    return reply::parse<long long>(*reply);
}

template <typename Interval>
long long Redis::zlexcount(const StringView &key, const Interval &interval) {
    auto reply = command(cmd::zlexcount<Interval>, key, interval);

    return reply::parse<long long>(*reply);
}

template <typename Output>
void Redis::zrange(const StringView &key, long long start, long long stop, Output output) {
    auto reply = command(cmd::zrange<Output>, key, start, stop);

    reply::to_score_array(*reply, output);
}

template <typename Interval, typename Output>
void Redis::zrangebylex(const StringView &key, const Interval &interval, Output output) {
    zrangebylex(key, interval, {}, output);
}

template <typename Interval, typename Output>
void Redis::zrangebylex(const StringView &key,
                        const Interval &interval,
                        const LimitOptions &opts,
                        Output output) {
    auto reply = command(cmd::zrangebylex<Interval>, key, interval, opts);

    reply::to_array(*reply, output);
}

template <typename Interval, typename Output>
void Redis::zrangebyscore(const StringView &key,
                            const Interval &interval,
                            Output output) {
    zrangebyscore(key, interval, {}, output);
}

template <typename Interval, typename Output>
void Redis::zrangebyscore(const StringView &key,
                            const Interval &interval,
                            const LimitOptions &opts,
                            Output output) {
    auto reply = command(cmd::zrangebyscore<Interval, Output>,
                            key,
                            interval,
                            opts);

    reply::to_score_array(*reply, output);
}

template <typename Input>
long long Redis::zrem(const StringView &key, Input first, Input last) {
    auto reply = command(cmd::zrem_range<Input>, key, first, last);

    return reply::parse<long long>(*reply);
}

template <typename Interval>
long long Redis::zremrangebylex(const StringView &key, const Interval &interval) {
    auto reply = command(cmd::zremrangebylex<Interval>, key, interval);

    return reply::parse<long long>(*reply);
}

template <typename Interval>
long long Redis::zremrangebyscore(const StringView &key, const Interval &interval) {
    auto reply = command(cmd::zremrangebyscore<Interval>, key, interval);

    return reply::parse<long long>(*reply);
}

template <typename Output>
void Redis::zrevrange(const StringView &key, long long start, long long stop, Output output) {
    auto reply = command(cmd::zrevrange<Output>, key, start, stop);

    reply::to_score_array(*reply, output);
}

template <typename Interval, typename Output>
inline void Redis::zrevrangebylex(const StringView &key,
                                    const Interval &interval,
                                    Output output) {
    zrevrangebylex(key, interval, {}, output);
}

template <typename Interval, typename Output>
void Redis::zrevrangebylex(const StringView &key,
                            const Interval &interval,
                            const LimitOptions &opts,
                            Output output) {
    auto reply = command(cmd::zrevrangebylex<Interval>, key, interval, opts);

    reply::to_array(*reply, output);
}

template <typename Interval, typename Output>
void Redis::zrevrangebyscore(const StringView &key, const Interval &interval, Output output) {
    zrevrangebyscore(key, interval, {}, output);
}

template <typename Interval, typename Output>
void Redis::zrevrangebyscore(const StringView &key,
                                const Interval &interval,
                                const LimitOptions &opts,
                                Output output) {
    auto reply = command(cmd::zrevrangebyscore<Interval, Output>, key, interval, opts);

    reply::to_score_array(*reply, output);
}

template <typename Output>
long long Redis::zscan(const StringView &key,
                        long long cursor,
                        const StringView &pattern,
                        long long count,
                        Output output) {
    auto reply = command(cmd::zscan, key, cursor, pattern, count);

    return detail::parse_scan_reply(*reply, output);
}

template <typename Output>
inline long long Redis::zscan(const StringView &key,
                                long long cursor,
                                const StringView &pattern,
                                Output output) {
    return zscan(key, cursor, pattern, 10, output);
}

template <typename Output>
inline long long Redis::zscan(const StringView &key,
                                long long cursor,
                                long long count,
                                Output output) {
    return zscan(key, cursor, "*", count, output);
}

template <typename Output>
inline long long Redis::zscan(const StringView &key,
                                long long cursor,
                                Output output) {
    return zscan(key, cursor, "*", 10, output);
}

template <typename Input>
long long Redis::zunionstore(const StringView &destination,
                                    Input first,
                                    Input last,
                                    Aggregation type) {
    auto reply = command(cmd::zunionstore<Input>,
                            destination,
                            first,
                            last,
                            type);

    return reply::parse<long long>(*reply);
}

// HYPERLOGLOG commands.

template <typename Input>
bool Redis::pfadd(const StringView &key, Input first, Input last) {
    auto reply = command(cmd::pfadd_range<Input>, key, first, last);

    return reply::parse<bool>(*reply);
}

template <typename Input>
long long Redis::pfcount(Input first, Input last) {
    auto reply = command(cmd::pfcount_range<Input>, first, last);

    return reply::parse<long long>(*reply);
}

template <typename Input>
void Redis::pfmerge(const StringView &destination,
                    Input first,
                    Input last) {
    auto reply = command(cmd::pfmerge<Input>, destination, first, last);

    reply::expect_ok_status(*reply);
}

// GEO commands.

template <typename Input>
inline long long Redis::geoadd(const StringView &key,
                                Input first,
                                Input last) {
    auto reply = command(cmd::geoadd_range<Input>, key, first, last);

    return reply::parse<long long>(*reply);
}

template <typename Input, typename Output>
void Redis::geohash(const StringView &key, Input first, Input last, Output output) {
    auto reply = command(cmd::geohash_range<Input>, key, first, last);

    reply::to_array(*reply, output);
}

template <typename Input, typename Output>
void Redis::geopos(const StringView &key, Input first, Input last, Output output) {
    auto reply = command(cmd::geopos_range<Input>, key, first, last);

    reply::to_array(*reply, output);
}

template <typename Output>
void Redis::georadius(const StringView &key,
                        const std::pair<double, double> &loc,
                        double radius,
                        GeoUnit unit,
                        long long count,
                        bool asc,
                        Output output) {
    auto reply = command(cmd::georadius<Output>, key, loc, radius, unit, count, asc);

    reply::to_array(*reply, output);
}

template <typename Output>
void Redis::georadiusbymember(const StringView &key,
                                const StringView &member,
                                double radius,
                                GeoUnit unit,
                                long long count,
                                bool asc,
                                Output output) {
    auto reply = command(cmd::georadiusbymember<Output>, key, member, radius, unit, count, asc);

    reply::to_array(*reply, output);
}

// SCRIPTING commands.

template <typename Result>
Result Redis::eval(const StringView &script,
                    std::initializer_list<StringView> keys,
                    std::initializer_list<StringView> args) {
    auto reply = command(cmd::eval, script, keys, args);

    return reply::parse<Result>(*reply);
}

template <typename Result, typename Output>
void Redis::eval(const StringView &script,
                    std::initializer_list<StringView> keys,
                    std::initializer_list<StringView> args,
                    Output output) {
    auto reply = command(cmd::eval, script, keys, args);

    reply::to_array(*reply, output);
}

template <typename Result>
Result Redis::evalsha(const StringView &script,
                        std::initializer_list<StringView> keys,
                        std::initializer_list<StringView> args) {
    auto reply = command(cmd::evalsha, script, keys, args);

    return reply::parse<Result>(*reply);
}

template <typename Result, typename Output>
void Redis::evalsha(const StringView &script,
                        std::initializer_list<StringView> keys,
                        std::initializer_list<StringView> args,
                        Output output) {
    auto reply = command(cmd::evalsha, script, keys, args);

    reply::to_array(*reply, output);
}

template <typename Input, typename Output>
void Redis::script_exists(Input first, Input last, Output output) {
    auto reply = command(cmd::script_exists_range<Input>, first, last);

    reply::to_array(*reply, output);
}

}

}

#endif // end SEWENEW_REDISPLUSPLUS_REDIS_HPP
