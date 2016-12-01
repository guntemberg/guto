// Relógio Ajustável (RTC + LCD + Teclado)

#include <Wire.h>			// Habilita uso da comunicação I2C
#include "LCD5110.h"		// Caracteres do Display Nokia 5110
#include <Keypad.h>			// Facilita o uso do teclado

#define	enderecoRTC	0x68	// Endereço I2C do módulo RTC

String ajustaSegundo, ajustaMinuto, ajustaHora, dia, ajustaDiaDoMes, ajustaMes, ajustaAno;	// Adiciona o zero quando < 9
byte segundo, minuto, hora, diaDaSemana, diaDoMes, mes, ano;								// Armazena valor lido do RTC

bool editar = false;	// Verifica se modo de Ajuste está ativo
byte tmp = 0x00;		// Armazena valor digitado antes de ir para o RTC
byte edicao = 0x01;		// Identifica qual campo está sendo editado (hora, minuto, segundo, dia...)

#define	PIN_SCE		7		// Pinos usados no display
#define	PIN_RESET	6
#define	PIN_DC		5
#define	PIN_SDIN	4
#define	PIN_SCLK	3
#define	LCD_C		LOW		// Indica que LCD irá receber comandos
#define	LCD_D		HIGH	// Indica que LCD irá receber dados

const byte ROWS = 4;		// Teclado de 4 linhas...
const byte COLS = 4;		// ...e três colunas

char keys[ROWS][COLS] = {	// Dígitos do teclado
	{'1','2','3', 'A'},
	{'4','5','6', 'B'},
	{'7','8','9', 'C'},
	{'*','0','#', 'D'}
};

byte rowPins[ROWS] = {11, 10, 9, 8};		// Pinos usados para conectar as linhas do teclado
byte colPins[COLS] = {14, 15, 16, 17};		// Pinos usados para conectar as colunas do teclado

Keypad teclado = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

void setup(){
	Wire.begin();							// Inicializa protocolo I2C
	Serial.begin(115200);					// Inicializa Serial (usada apenas na depuração do código)
	teclado.addEventListener(keypadEvent);	// Detecta alguma alteração do estado do teclado

	inicializaLCD();						// Inicializa LCD
}

void loop(){
	char key = teclado.getKey();			// Verifica se houve alteração no estado do teclado

	imprimeRTC();							// Lê segundo, minuto, hora, dia, mês...

	gotoXY(8, 0);							// Move o cursor do display para a posição 8 da coluna 0
	LCDPrintStr(ajustaHora, 0x01);			// Imprime texto da variável ajusta hora e envia posição desse texto no LCD
	LCDCharacter(':', false);				// Imprime dois pontos no display
	LCDPrintStr(ajustaMinuto, 0x02);
	LCDCharacter(':', false);
	LCDPrintStr(ajustaSegundo, 0x04);
	
	gotoXY(8, 2);
	LCDPrintStr(dia, 0x08);

	gotoXY(8, 3);
	LCDPrintStr(ajustaDiaDoMes, 0x10);
	LCDCharacter('/', false);
	LCDPrintStr(ajustaMes, 0x20);
	LCDCharacter('/', false);
	LCDPrintStr(ajustaAno, 0x40);
}

///// Teclado /////
void keypadEvent(KeypadEvent key){			// Função para lidar com a mudança de estado do teclado
	switch(teclado.getState()){
		case PRESSED:
			if(key == '*'){					// Se a tecla * foi pressionada...
				if(edicao > 0x01)			// ...e o campo atual não é o primeiro...
					edicao = edicao >> 1;	// ...então move o ajuste para o campo anterior

				tmp = 0x00;
				Serial.println(">>");
			}
			else if(key == '#'){			// Se a tecla # foi pressionada...
				edicao = edicao << 1;		// ...move o campo de ajuste para o seguinte

				if(edicao > 0x40)			// Se o campo de ajuste era o último...
					edicao = 0x01;			// ...então salta para o primeiro

				tmp = 0x00;
				Serial.println("<<");
			}
			else if(key == 'A'){			// Se as teclas A, B, C ou D forem pressionadas...
			}								// ...nenhuma ação é realizada
			else if(key == 'B'){
			}
			else if(key == 'C'){
			}
			else if(key == 'D'){
			}
			else {								// Se uma tecla numérica for pressionada, move o valor anterior...
				tmp = (tmp << 4) + (key - 0x30);// ...4 bits para esquerda e adiciona o valor lido da tecla
				Serial.print("0x");
				atualizaRTC();					// Chama função mostrar no display o valor digitado no teclado
				Serial.println(tmp, HEX);
			}
		break;

		case RELEASED:		// Quando uma tecla é liberada, nenhuma ação é realizada
		break;

		case HOLD:						// Se uma tecla for mantida pressionada...
			if(key == 'A'){				// ...e essa tecla for A, então...
				editar = true;			// ...modo de edição é ativado...
				edicao = 0x01;			// ...na posição do primeiro campo (hora)...
				tmp = 0x00;
				Serial.println("Ajuste ON");
			}
			else if(key == 'C'){		// ...se a tecla for C, então...
				editar = false;			// ...finaliza modo de edição e...
				edicao = 0x01;			// ...move cursor para primeira posição
				tmp = 0x00;
				Serial.println("Ajuste OFF");
			}
		break;
	}
}

/////// LCD ///////
void inicializaLCD(){
	pinMode(PIN_SCE, OUTPUT);		// Declara pinos usados no LCD como saídas
	pinMode(PIN_RESET, OUTPUT);
	pinMode(PIN_DC, OUTPUT);
	pinMode(PIN_SDIN, OUTPUT);
	pinMode(PIN_SCLK, OUTPUT);
	digitalWrite(PIN_RESET, LOW);	// Reinicia LCD
	digitalWrite(PIN_RESET, HIGH);

	LCDWrite(LCD_C, 0x21);			// Indica ao LCD que virão alguns comandos
	LCDWrite(LCD_C, 0xB5);			// Contraste do LCD, entre 0xB1 e 0xBF
	LCDWrite(LCD_C, 0x04);			// Coeficiente de temperatura: 0x04
	LCDWrite(LCD_C, 0x14);			// LCD bias mode 1:48 //0x13 ou 0x14
	LCDWrite(LCD_C, 0x20);			// Envia-se 0x20 antes de entrar no modo de operação
	LCDWrite(LCD_C, 0x0C);			// LCD em modo normal (0x0D para modo inverso (preto))
	LCDClear();						// Limpa tela do display
}

void LCDWrite(byte dc, byte data){	// Escreve os pixels no teclado
	digitalWrite(PIN_DC, dc);
	digitalWrite(PIN_SCE, LOW);
	shiftOut(PIN_SDIN, PIN_SCLK, MSBFIRST, data);
	digitalWrite(PIN_SCE, HIGH);
}

void LCDClear(){
	for(int i = 0; i < 84 * 48 / 8; i++)
		LCDWrite(LCD_D, 0x00);

	gotoXY(0, 0);					// Sem essa função o texto vai rolar pela tela
}

void gotoXY(int x, int y){
	LCDWrite(0, 0x80 | x);			// Coluna
	LCDWrite(0, 0x40 | y);			// Linha
}

void LCDString(char *characters){
	while(*characters)
		LCDCharacter(*characters++, false);
}

void LCDPrintStr(String str, byte inverter){
	int i = 0;

	while(str[i] != 0x00)
		LCDCharacter(str[i++], (editar && (inverter == edicao)));
}

void LCDCharacter(char character, bool inverter){
	if(inverter){
		LCDWrite(LCD_D, 0xFF);	// Imprime coluna "escura" antes do dígito

		for(int i = 0; i < 5; i++)
			LCDWrite(LCD_D, ~ASCII[character - 0x20][i]);

		LCDWrite(LCD_D, 0xFF);	// Imprime coluna "escura" depois do dígito
	}
	else {
		LCDWrite(LCD_D, 0x00);	// Imprime coluna "em branco" antes do dígito

		for(int i = 0; i < 5; i++)
			LCDWrite(LCD_D, ASCII[character - 0x20][i]);

		LCDWrite(LCD_D, 0x00);	// Imprime coluna "em branco" depois do dígito
	}
}

/////// RTC ///////
void atualizaRTC(){
	byte endereco = 0x00;

	switch(edicao){
		case 0x01: endereco = 0x02; break;	// Campo das Horas
		case 0x02: endereco = 0x01; break;	// Campo dos Minutos
		case 0x04: endereco = 0x00; break;	// Campo dos Segundos
		case 0x08: endereco = 0x03; break;	// Campo do Dia da semana
		case 0x10: endereco = 0x04; break;	// Campo do Dia do mês
		case 0x20: endereco = 0x05; break;	// Campo do Mês
		case 0x40: endereco = 0x06; break;	// Campo do Ano
		default: return;
	}

	switch(endereco){
		case 0x00: if(tmp > 0x59) tmp = 0x00; break;	// Ajusta valor máximo dos Segundos
		case 0x01: if(tmp > 0x59) tmp = 0x00; break;	// Ajusta valor máximo dos Minutos
		case 0x02: if(tmp > 0x23) tmp = 0x00; break;	// Ajusta valor máximo das Horas
		case 0x03: if(tmp > 0x06) tmp = 0x00; break;	// Ajusta valor máximo do Dia da semana
		case 0x04: if(tmp > 0x31) tmp = 0x01; break;	// Ajusta valor máximo do Dia do mês
		case 0x05: if(tmp > 0x12) tmp = 0x01; break;	// Ajusta valor máximo do Mês
		case 0x06: if(tmp > 0x99) tmp = 0x00; break;	// Ajusta valor máximo do Ano
		default: return;
	}

	Wire.beginTransmission(enderecoRTC);	// Inicia transmissão de dados para RTC
	Wire.write(endereco);					// Endereço do primeiro registrador a ser esrito
	Wire.write(tmp);						// Dado a ser escrito no RTC
	Wire.endTransmission(true);				// Finaliza comunicação com RTC
}

void imprimeRTC(){							// Ler dados do RTC
	Wire.beginTransmission(enderecoRTC);
	Wire.write(0x00);						// Endereço do primeiro registrador a ser escrito
	Wire.endTransmission();
	Wire.requestFrom(enderecoRTC, 7);		// Solicita dados do sétimo registrador do RTC

	segundo = bcdToDec(Wire.read() & 0x7f);	// Lê segundos em BCD e converte para decimal
	minuto = bcdToDec(Wire.read());
	hora = bcdToDec(Wire.read() & 0x3f);
	diaDaSemana = bcdToDec(Wire.read());
	diaDoMes = bcdToDec(Wire.read());
	mes = bcdToDec(Wire.read());
	ano = bcdToDec(Wire.read());

	ajustaHora = ajustaZero(hora);			// Acrescenta zero antes do dígito caso o número seja menor que 9
	ajustaMinuto = ajustaZero(minuto);
	ajustaSegundo = ajustaZero(segundo);

	switch(diaDaSemana){					// Switch-case para identificar o dia da semana
		case 0: dia = "Domingo"; break;
		case 1: dia = "Segunda"; break;
		case 2: dia = "Terca  "; break;
		case 3: dia = "Quarta "; break;
		case 4: dia = "Quinta "; break;
		case 5: dia = "Sexta  "; break;
		case 6: dia = "Sabado "; break;
		default: dia = "Erro!";				// Caso seja um valor inválio, imprime a palavra "Erro!"
	}

	ajustaDiaDoMes = ajustaZero(diaDoMes);
	ajustaMes = ajustaZero(mes);
	ajustaAno = ajustaZero(ano);
}

byte decToBcd(byte val){					// Converte decimal para BCD
	return ((val/10*16) + (val%10));
}

byte bcdToDec(byte val){					// Converte BCD para Decimal
	return ((val/16*10) + (val%16));
}

String ajustaZero(byte dado){
	String dadoAjustado;

	if (dado < 10)					// Se o número for menor que 9...
		dadoAjustado += "0";		// ...acrescenta zero

	return (dadoAjustado + dado);
}
