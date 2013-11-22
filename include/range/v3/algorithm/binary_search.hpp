//  Copyright Neil Groves 2009. 
//  Copyright Eric Niebler 2013
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/range/
//
#ifndef RANGES_V3_ALGORITHM_BINARY_SEARCH_HPP
#define RANGES_V3_ALGORITHM_BINARY_SEARCH_HPP

#include <utility>
#include <algorithm>
#include <range/v3/range_fwd.hpp>
#include <range/v3/range_traits.hpp>
#include <range/v3/concepts.hpp>
#include <range/v3/utility/bindable.hpp>

namespace ranges
{
    inline namespace v3
    {
        struct binary_searcher
        {
            /// \brief function template \c binary_searcher::operator()
            ///
            /// range-based version of the \c binary_search std algorithm
            ///
            /// \pre \c ForwardRange is a model of the ForwardRange concept
            /// \pre \c BinaryPredicate is a model of the BinaryPredicate concept
            template<typename ForwardRange, typename Value>
            bool operator()(ForwardRange && rng, Value const & val) const
            {
                static_assert(ranges::ForwardRange<ForwardRange>(),
                    "Expecting model of ForwardRange");
                return std::binary_search(ranges::begin(rng), ranges::end(rng), val);
            }

            /// \overload
            template<typename ForwardRange, typename Value, typename BinaryPredicate>
            bool operator()(ForwardRange && rng, Value const & val, BinaryPredicate pred) const
            {
                static_assert(ranges::ForwardRange<ForwardRange>(),
                    "Expecting model of ForwardRange");
                static_assert(
                    ranges::BinaryPredicate<BinaryPredicate,
                                            range_reference_t<ForwardRange>,
                                            range_reference_t<ForwardRange>>(),
                    "Expecting model of BinaryPredicate");
                return std::binary_search(ranges::begin(rng), ranges::end(rng), val, detail::move(pred));
            }
        };

        constexpr bindable<binary_searcher> binary_search {};

    } // namespace v3
} // namespace ranges

#endif // include guard