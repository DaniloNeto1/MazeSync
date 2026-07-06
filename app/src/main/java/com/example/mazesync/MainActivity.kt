package com.example.mazesync

import android.content.Context
import android.hardware.Sensor
import android.hardware.SensorEvent
import android.hardware.SensorEventListener
import android.hardware.SensorManager
import android.os.Bundle
import android.widget.TextView
import androidx.activity.enableEdgeToEdge
import androidx.appcompat.app.AppCompatActivity
import androidx.core.view.ViewCompat
import androidx.core.view.WindowInsetsCompat
import java.net.DatagramPacket
import java.net.DatagramSocket
import java.net.InetAddress
import kotlin.concurrent.thread

class MainActivity : AppCompatActivity(), SensorEventListener {

    // Gerenciamento de Sensores
    private lateinit var sensorManager: SensorManager
    private var gravidadeSensor: Sensor? = null

    // Armazena a última posição
    private var ultimoAnguloX = 90
    private var ultimoAnguloY = 90

    // O ESP32 agora cria a própria rede Wi-Fi no modo Access Point.
    // No modo AP, o IP padrão configurado abaixo no ESP32 é 192.168.4.1.
    private val esp32Ip = "192.168.4.1"
    private val esp32Porta = 12345
    private val esp32RedeWifi = "MazeSync-ESP32"
    private val esp32SenhaWifi = "12345678"

    private lateinit var tvStatus: TextView
    private lateinit var tvEixoX: TextView
    private lateinit var tvEixoY: TextView

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        // Mantém a interface moderna (tela cheia)
        enableEdgeToEdge()
        setContentView(R.layout.activity_main)
        ViewCompat.setOnApplyWindowInsetsListener(findViewById(R.id.main)) { v, insets ->
            val systemBars = insets.getInsets(WindowInsetsCompat.Type.systemBars())
            v.setPadding(systemBars.left, systemBars.top, systemBars.right, systemBars.bottom)
            insets
        }

        // Mapeamento dos textos da tela
        tvStatus = findViewById(R.id.tvStatus)
        tvEixoX = findViewById(R.id.tvEixoX)
        tvEixoY = findViewById(R.id.tvEixoY)

        // Inicializa o leitor de movimento
        sensorManager = getSystemService(Context.SENSOR_SERVICE) as SensorManager
        gravidadeSensor = sensorManager.getDefaultSensor(Sensor.TYPE_GRAVITY)

        if (gravidadeSensor == null) {
            tvStatus.text = "ERRO: O celular não possui sensor de gravidade!"
            tvStatus.setTextColor(android.graphics.Color.RED)
        } else {
            tvStatus.text = "Sensor OK.\nConecte o Android no Wi-Fi '$esp32RedeWifi'.\nSenha: $esp32SenhaWifi"
        }
    }

    override fun onResume() {
        super.onResume()
        gravidadeSensor?.let {
            sensorManager.registerListener(this, it, SensorManager.SENSOR_DELAY_GAME)
        }
    }

    override fun onPause() {
        super.onPause()
        sensorManager.unregisterListener(this)
    }

    override fun onSensorChanged(event: SensorEvent?) {
        if (event == null || event.sensor.type != Sensor.TYPE_GRAVITY) return

        val gX = event.values[0]
        val gY = event.values[1]

        // Cálculo dos ângulos (mantendo a segurança mecânica entre 58 e 118)
        val anguloX = (90 + (gX * 7.0)).toInt().coerceIn(58, 118)
        val anguloY = (90 + (gY * 7.0)).toInt().coerceIn(58, 118)

        // Se o valor mudou, atualiza a tela e envia pela rede
        if (anguloX != ultimoAnguloX || anguloY != ultimoAnguloY) {
            ultimoAnguloX = anguloX
            ultimoAnguloY = anguloY

            tvEixoX.text = "$anguloX°"
            tvEixoY.text = "$anguloY°"

            tvStatus.text = "Transmitindo para $esp32RedeWifi ($esp32Ip:$esp32Porta)..."

            transmitirDadosUdp(anguloX, anguloY)
        }
    }

    override fun onAccuracyChanged(sensor: Sensor?, accuracy: Int) {
        // Sem uso
    }

    // Comunicação paralela enviando o formato "X,Y" esperado pelo sscanf do ESP32
    private fun transmitirDadosUdp(angX: Int, angY: Int) {
        thread {
            try {
                // Monta a string "X,Y" (Ex: "100,80")
                val payload = "$angX,$angY"
                val buffer = payload.toByteArray(Charsets.UTF_8)

                // Envia para o IP fixo do ESP32 quando ele está em modo Access Point
                val enderecoDestino = InetAddress.getByName(esp32Ip)

                DatagramSocket().use { socket ->
                    val pacote = DatagramPacket(buffer, buffer.size, enderecoDestino, esp32Porta)
                    socket.send(pacote)
                }
            } catch (e: Exception) {
                runOnUiThread {
                    tvStatus.text = "Erro na conexão: ${e.message}"
                    tvStatus.setTextColor(android.graphics.Color.RED)
                }
                e.printStackTrace()
            }
        }
    }
}
