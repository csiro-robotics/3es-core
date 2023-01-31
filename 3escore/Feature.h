//
// author: Kazys Stepanas
//
//
#ifndef TES_CORE_FEATURE_H
#define TES_CORE_FEATURE_H

#include "CoreConfig.h"

#include <cinttypes>

namespace tes
{
/// Defines the set of feature flags.
/// See @c checkFeature().
enum Feature
{
  /// Is compression is available.
  TFeatureCompression,

  /// Notes the number of valid feature values.
  /// While @c TFeatureLimit shows the maximum possible features we can track,
  /// This is the maximum we can actually express and ever need iterate for.
  TFeatureEnd,
  TFeatureLimit = 64,
  TFeatureInvalid = TFeatureLimit
};

/// Convert a @c Feature to a feature flag.
///
/// This is simply <tt>1 << feature</tt>.
/// @param feature The feature of interest.
/// @return The flag for @p feature.
uint64_t TES_CORE_API featureFlag(Feature feature);

/// Convert a feature flag back to a @c feature.
///
/// Only the first feature flag is noted when multiple bits are set.
/// @param flag The feature flag of interest.
/// @return The @c Feature associated with @p flag or @c TFeatureInvalid if no
///   valid feature bits are set.
/// @see @c featureFlag()
Feature TES_CORE_API featureForFlag(uint64_t flag);

/// Check if a particular @c Feature is available.
/// @param feature The feature to check for.
/// @return True if the feature is available or enabled.
bool TES_CORE_API checkFeature(Feature feature);

/// Check for a feature by its flag.
///
/// Similar to @c checkFeature(), except that it uses the feature flag.
/// For this method, only 1 bit can be set or the result is always false.
///
/// @param featureFlag The feature flag to check for.
/// @return True if the feature is available.
bool TES_CORE_API checkFeatureFlag(uint64_t featureFlag);

/// Check if a set of features are available. Use @c featureFlag() to convert from
/// @c Feature to a feature flag.
///
/// Only valid feature flags are checked. Always true if @p featureFlags is zero.
/// @param featureFlags Set of features to check for.
/// @return True if all features in @p featureFlags are available.
bool TES_CORE_API checkFeatures(uint64_t featureFlags);
}  // namespace tes

#endif  // TES_CORE_FEATURE_H
