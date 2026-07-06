//bibliotecas
#include <Arduino.h> //para uso de funçoes como pinMode, delay...
#include <ESP32Servo.h> //para o controle dos servos com ESP32
#include <WiFi.h> //para conexão WiFi do ESP
#include <WiFiUdp.h> //para comunicaçao UDP com WiFi

// O ESP32 agora cria a própria rede Wi-Fi no modo Access Point.
// O Android deve se conectar nessa rede e enviar UDP para 192.168.4.1:12345.
const char* AP_SSID = "MazeSync-ESP32";
const char* AP_PASSWORD = "12345678";

IPAddress AP_LOCAL_IP(192, 168, 4, 1);
IPAddress AP_GATEWAY(192, 168, 4, 1);
IPAddress AP_SUBNET(255, 255, 255, 0);

//porta UDP para conexão do celular com ESP
const unsigned int udpPort=12345;

WiFiUDP udp; //gerenciador da comunicação UDP
char buffer[255]; //armazena a mensagem recebida do celular

//pinagem
//pinos para LEITURA do joystick. Tensão convertida em um valor de 0 a 4095
#define PIN_JOY_X 34 //eixo X do joy (esquerda direita)
#define PIN_JOY_Y 35 //eixo Y do joy (frente/tras)
#define PIN_JOY_SW 32 //botão do joy que quando pressionado tabuleiro fica em 90 graus (low)

//pinos de SAIDA para os servos (PWM)
#define PIN_SERVO_X 18 // servo que vai inclinar o labirinto em X
#define PIN_SERVO_Y 19 //servo que vai inclinar o labirinto em Y

//configuração dos servos 
//definição de angulos limites 
//#define SERVO_CENTER 70 //posição central, em 90 graus nivelado
#define SERVO_CENTER_X  88   // ajuste este se o eixo X também estiver torto
#define SERVO_CENTER_Y  88   // ALTERE ESTE — diminua se inclina para frente, aumente se para trá

#define SERVO_MIN 58 //angulo minimo (inclinação maxima -)
#define SERVO_MAX 118 //angulo maximo (maximo em +) 

//configuração do joystick
//gera valores de 0 a 4095, o centro é 1950
#define JOY_CENTER 1950 //valor do ADC quando joystick parado
#define JOY_DEADZONE 150 //faz com que o labirinto sofra com ruidos. Reage se o valor sair da faixa de 1800 a 2100

//configuração do incremento
#define INCREMENTO_MAX 1.5f //angulo muda 2.5 graus por ciclo (maximo)
#define INCREMENTO_MIN 0.2f //muda 0.3 por ciclo (saindo da deadzone)
#define LOOP_DELAY_MS 15 //loop roda a cada 15milisegundos

//variaveis globais/prototipos
Servo servoX; //obejto que representa e controla o servo do eixo X
Servo servoY; //obejto que representa e controla o servo do eixo Y

//angulos atuais do servo. Saem do centro
float anguloX = SERVO_CENTER_X;
float anguloY = SERVO_CENTER_Y;

//prototipos de funcoes 
float calcularIncremento(int valorADC); 
float limitarAngulo(float angulo);
void resetarCentro();
void iniciarWifiAccessPoint();

//setup
void setup(){
  Serial.begin(115200); //inicio da comunicação serial

  //configuracao do pino do botao como ENTRADA. Resistor pull-up (padrao é high e quando pressionado vai para low)
  pinMode(PIN_JOY_SW, INPUT_PULLUP);

  //configuração de timers para gerar o pwm dos servos. Um timer para cada servo,
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);

  //configuracao dos servos
  servoX.setPeriodHertz(50); //padrao 
  servoY.setPeriodHertz(50);
  servoX.attach(PIN_SERVO_X, 500, 2400); //
  servoY.attach(PIN_SERVO_Y, 500, 2400);

  resetarCentro(); //os dois servos no centro
  delay(500); //espera 500 milisegundos para o servo chegar ao centro

  iniciarWifiAccessPoint();
  udp.begin(udpPort);

  Serial.println("UDP iniciado.");
  Serial.print("Porta UDP: ");
  Serial.println(udpPort);
}

//loop
void loop(){
  //botao pressionado entao centraliza. Prioridade 1
  if(digitalRead(PIN_JOY_SW)==LOW){
    resetarCentro();
    delay(300); //tratamento para nao registrar o botao 2 vezes
    return; //nao faz o resto do loop e comeca no outro ciclo
  }

  //leitura do joy, prioridade 2
  int leituraX=analogRead(PIN_JOY_X); 
  int leituraY=analogRead(PIN_JOY_Y); 

  //calculo de mudança do angulo baseado na posição do joy
  float deltaX=calcularIncremento(leituraX); 
  float deltaY=calcularIncremento(leituraY); 

  //se joy se moveu (delta diferente de 0), contole com joy
  if(abs(deltaX)>0.01f||abs(deltaY)>0.01f){
    anguloX=limitarAngulo(anguloX+deltaX); //soma incremento do angulo atual
    anguloY=limitarAngulo(anguloY+deltaY);
  }
  else{ //joy parado controle via celular. Prioridade 3
    int packetSize=udp.parsePacket(); //ve se chegou mensagem
    if(packetSize){
      int len=udp.read(buffer, sizeof(buffer)-1); //le a mensagem do buffer com espaço para ' '
      if(len>0){
        buffer[len]=0;
      }

      int novoX, novoY;
      if(sscanf(buffer, "%d,%d", &novoX, &novoY)==2){
        anguloX=limitarAngulo((float)novoX);
        anguloY=limitarAngulo((float)novoY);

        Serial.print("Recebido do Android: ");
        Serial.println(buffer);
      }
    }
  }

  //envia angulos calculados para os servos
  servoX.write((int)anguloX);
  servoY.write((int)anguloY);

  //debug no Serial Monitor
  Serial.printf("JoyX: %4d | JoyY: %4d | ServX: %5.1f | ServY: %5.1f", leituraX, leituraY, anguloX, anguloY);

  delay(LOOP_DELAY_MS);
}

void iniciarWifiAccessPoint(){
  WiFi.disconnect(true); //limpa configurações antigas da memória
  WiFi.mode(WIFI_AP); //configuracao do ESP32 para operar no modo Access Point

  //aplica configuracoes de IP fixo, Gateway e Máscara
  if(!WiFi.softAPConfig(AP_LOCAL_IP, AP_GATEWAY, AP_SUBNET)){
    Serial.println("Falha ao configurar IP fixo do Access Point."); //aviso caso falha
  }

  //inicializa a rede Wi-Fi local gerando o SSID (nome) e a senha protegida por WPA2
  bool apOk = WiFi.softAP(AP_SSID, AP_PASSWORD);

  //validacao da rede
  if(apOk){
    Serial.println("Access Point iniciado.");
    Serial.print("Rede Wi-Fi: ");
    Serial.println(AP_SSID);
    Serial.print("Senha: ");
    Serial.println(AP_PASSWORD);
    Serial.print("IP do ESP32: ");
    Serial.println(WiFi.softAPIP());
  }
  else{
    Serial.println("ERRO: falha ao iniciar Access Point do ESP32."); //aviso caso falha
  }
}

float calcularIncremento(int valorADC) {
  int desvio=valorADC-JOY_CENTER; //distância entre leitura atual do ADC (0-4095) e o centro (1950)

  //quando o valor absoluto do desvio for menor que a zona morta (150) o joystick nao move
  if (abs(desvio)<JOY_DEADZONE){
    return 0.0f;
  }
  
  float desvioUtil=abs(desvio)-JOY_DEADZONE; //remove a regiao da zona morta do desvio para obter o deslocamento pedido pelo usuário
  float desvioMax=(float)(JOY_CENTER-JOY_DEADZONE); //desvio maximo possivel a partir do centro
  float normalizado=constrain(desvioUtil/desvioMax, 0.0f, 1.0f); //normaliza o desvio em uma escala de 0.0 (início do movimento) a 1.0 (fim do movimento)

  float incremento=INCREMENTO_MIN+normalizado*(INCREMENTO_MAX-INCREMENTO_MIN); //velocidade do incremento angular

  if (desvio>0){
    return incremento; //se o desvio foi positivo joystick para a direita/frente
  } else{
    return -incremento; //senao joystick para a esquerda/trás
  }
}

float limitarAngulo(float angulo) {
  if (angulo<SERVO_MIN){
    return (float)SERVO_MIN;
  }
  
  if (angulo>SERVO_MAX){
    return (float)SERVO_MAX;
  }
 
  return angulo;
}

void resetarCentro() {
  anguloX = SERVO_CENTER_X;  // era SERVO_CENTER (90°)
  anguloY = SERVO_CENTER_Y;  // era SERVO_CENTER (90°)
  servoX.write(SERVO_CENTER_X);  // era SERVO_CENTER (90°)
  servoY.write(SERVO_CENTER_Y);  // era SERVO_CENTER (90°)
}