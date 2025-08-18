#pragma once

namespace Interpolation {
    /* ----- The transition functions -----
     * The equations can be found on easings.net
     */

    // The currently supported transition functions
    enum class TransitionFunction {
        None,
        Linear,
        EaseInOutExponential,
        EaseOutBack,
        EaseInBack,
        EaseOutElastic,
    };

    // Easing function implementations (declarations in header)
    float linear(float t);
    float easeInOutExponential(float t);
    float easeOutBack(float t);
    float easeInBack(float t);
    float easeOutElastic(float t);

    // Calls the easing function associated with the provided enum entry
    float getRatio(float t, TransitionFunction transition);
}
