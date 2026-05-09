package com.minefilter.launcher

class FilterEngine {
    companion object {
        init {
            System.loadLibrary("filter_engine")
        }
    }

    external fun enableFilter(enabled: Boolean): Boolean
    external fun setTargetFps(fps: Int)
    external fun setRenderScale(scale: Float)
}
