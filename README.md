# 🕹️ MazeSync

Controle interativo em tempo real de um labirinto mecânico físico utilizando comunicação via rede sem fio entre um microcontrolador e um smartphone. 

Este repositório adota a arquitetura de **Monorepo**, contendo tanto o código embarcado (Hardware) quanto o aplicativo móvel (Software).

## 📐 Arquitetura do Sistema

O sistema é dividido em duas frentes que se comunicam via protocolo **UDP** em uma rede local (Access Point gerado pelo hardware), garantindo baixíssima latência (aprox. 15ms) para movimentação fluida.

1. **📱 App Android (Kotlin):** Lê os sensores de gravidade do smartphone utilizando a *Sensor API* nativa do Android SDK e atira pacotes UDP em uma *Thread* paralela (Background) contendo as coordenadas $X$ e $Y$.
2. **⚙️ Hardware (C++ / ESP32):** Microcontrolador recebe os pacotes de rede e traduz os ângulos em modulação PWM para controlar os micro-servos do labirinto. Também inclui lógica de interpolação para controle manual via Joystick analógico.

---

## 🛠️ Tecnologias Utilizadas

* **Linguagens:** Kotlin (App), C/C++ (Hardware)
* **Ambientes:** Android Studio, VS Code (PlatformIO)
* **Protocolo de Rede:** UDP (User Datagram Protocol) via Wi-Fi `802.11 b/g/n`
* **Hardware:** ESP32, Servo Motores (SG90), Módulo Joystick Analógico

---

## 🚀 HOW TO: Como executar o projeto

### Pré-requisitos
* **Android Studio** instalado para compilar o app.
* **VS Code** com a extensão **PlatformIO** para gravar o ESP32.

### Passo 1: Configurando o Hardware (ESP32)
1. Abra a pasta `Hardware-ESP32` no VS Code (PlatformIO).
2. Conecte o ESP32 ao computador via cabo USB.
3. Clique no ícone de **Upload** (seta para a direita) na barra inferior do PlatformIO para compilar e gravar o firmware na placa.
4. Após gravado, o ESP32 iniciará automaticamente uma rede Wi-Fi (Access Point).

### Passo 2: Configurando o Software (App Android)
1. Abra a pasta raiz do projeto Android no **Android Studio**.
2. Conecte seu smartphone ao computador via cabo USB (com o modo de depuração USB ativado) ou use um emulador.
3. Clique em **Run 'app'** (ícone de Play verde) na barra superior.
4. O aplicativo será instalado e aberto no seu dispositivo.

### Passo 3: Sincronizando (O Jogo)
1. No seu smartphone, vá nas configurações de Wi-Fi e conecte-se à rede gerada pelo ESP32 (ex: `MazeSync_AP`).
2. Abra o aplicativo MazeSync.
3. Incline o celular! O aplicativo fará a leitura da gravidade com um fator de ganho calibrado ($K = 7.0$) e os motores do labirinto responderão em tempo real respeitando a janela mecânica segura de 60° a 120°.

---

## 👨‍💻 Autor

**Danilo Pietrobon Neto** Engenharia de Computação - UTFPR
**Monica P. O. Mackert**  Engenharia de Computação - UTFPR