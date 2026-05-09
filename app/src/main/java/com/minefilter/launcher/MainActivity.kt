package com.minefilter.launcher

import android.os.Bundle
import android.widget.Button
import android.widget.SeekBar
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity

class MainActivity : AppCompatActivity() {
    private lateinit var filterEngine: FilterEngine
    private lateinit var fpsText: TextView

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        filterEngine = FilterEngine()
        fpsText = findViewById(R.id.fpsText)

        val enableBtn = findViewById<Button>(R.id.enableBtn)
        val fpsSlider = findViewById<SeekBar>(R.id.fpsSlider)

        enableBtn.setOnClickListener {
            val enabled = filterEngine.enableFilter(true)
            fpsText.text = if (enabled) "Engine ENABLED" else "Engine FAILED"
        }

        fpsSlider.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
                val targetFps = 30 + progress
                filterEngine.setTargetFps(targetFps)
                filterEngine.setRenderScale(if (targetFps < 50) 0.75f else 1.0f)
                fpsText.text = "Target FPS: $targetFps"
            }
            override fun onStartTrackingTouch(seekBar: SeekBar?) {}
            override fun onStopTrackingTouch(seekBar: SeekBar?) {}
        })
    }
}
