
#include "ItemFilaMovimento.h"
#include "Ponto.h"
#include "Perna.h"
#include "MoverHexapod.h"

// direção em radianos
const float DIREITA = 0;
const float FRENTE = 1 / 2.0 * PI;
const float ESQUERDA = PI;
const float ATRAS = 3 / 2.0 * PI;

// sentido da rotação
const bool HORARIO = true;
const bool ANTIHORARIO = false;

// Permitir que os passos sejam executados separadamente
int _clockAnterior = 0;


const int _tempoPorPasso = 200;

// Porcentagem de tempo que cada passo demora para ser executado (A soma deve ser 1)
float _andarTempoPasso[] = { 0.01, 0.01, 0.18, 0.02, 0.05, 0.23, 0.10, 0.10, 0.10, 0.47 };
float _girarTempoPasso[] = { 0.20, 0.20, 0.20, 0.20, 0.20 };

byte _statusMovimento = 1;
const byte STATUS_MOVIMENTO_COMPUTAR_INICIO = 1;
const byte STATUS_MOVIMENTO_COMPUTAR_MOVIMENTO = 2;
const byte STATUS_MOVIMENTO_COMPUTAR_TERMINO = 3;

byte _passo = 0;
byte _totalPassos = 6;

int _clockInicioMovimento;


list <ItemFilaMovimento> _filaMovimento;

int _tempoAguardar = 0;

#pragma region Funções auxiliares


float dist(Ponto p1, Ponto p2)
{
	// d² = h² + v²
	return sqrtf(powf(fabsf(p2.X - p1.X), 2) + powf(fabsf(p2.Y - p1.Y), 2));
}

float leiDosCossenos(float ladoA, float ladoB, float ladoC)
{
	// ladoA² = ladoB² + ladoC² - 2 * ladoB * ladoC * cos(anguloA)
	// ladoA² - ladoB² - ladoC² = - 2 * ladoB * ladoC * cos(anguloA)
	// - ladoA² + ladoB² + ladoC² = 2 * ladoB * ladoC * cos(anguloA)
	// (- ladoA² + ladoB² + ladoC²) / (2 * ladoB * ladoC) = cos(anguloA)
	// anguloA = acos((- ladoA² + ladoB² + ladoC²) / (2 * ladoB * ladoC))

	return acosf((-powf(ladoA, 2) + powf(ladoB, 2) + powf(ladoC, 2)) / (2 * ladoB * ladoC));
}

float map(float valueIn, float minIn, float maxIn, float minOut, float maxOut)
{
	return (valueIn - minIn) / (maxIn - minIn) * (maxOut - minOut) + minOut;
}

#pragma endregion

Perna retornarAngulosPerna(Ponto pontaPerna)
{
	/*

	Convenções utilizadas:
	- ângulos						--> radianos
	- distâncias					--> centimetros
	- posição do motorOmbro			--> (0, 0)


	a (biceps)						--> distância do motorOmbro até o motorCotovelo
	b (antebraco)					--> distância do motorCotovelo até a ponta da perna (pontaPerna.X, pontaPerna.Y)
	alfa							--> ângulo formado entre "a" e a linha do horizonte
	beta							--> ângulo complementar ao ângulo entre "a" e "b" (ângulo raso menos ângulo entre "a" e "b")
	c (distanciaOmbroAtePontaPerna)	--> distância do motorOmbro até a ponta da perna

	A								--> ângulo contrário ao braço "a"
	B								--> ângulo contrário ao braço "b"
	C								--> ângulo contrário ao braço "c"

	*/

	float a_biceps = 10;
	float b_antebraco = 12;

	Ponto motorOmbro(0, 0);

	float c_distanciaOmbroAtePontaPerna = dist(motorOmbro, pontaPerna);

	float anguloA = leiDosCossenos(a_biceps, b_antebraco, c_distanciaOmbroAtePontaPerna);
	float anguloB = leiDosCossenos(b_antebraco, a_biceps, c_distanciaOmbroAtePontaPerna);
	float anguloC = leiDosCossenos(c_distanciaOmbroAtePontaPerna, a_biceps, b_antebraco);

	float beta = PI - anguloC;

	float distanciaX = fabsf(pontaPerna.X - motorOmbro.X);
	float distanciaY = fabsf(pontaPerna.Y - motorOmbro.Y);

	float alfa;

	if (pontaPerna.Y < 0)
	{
		float angulo_cComVertical = atan2f(distanciaX, distanciaY);

		alfa = angulo_cComVertical + anguloB - (1 / 2.0 * PI);
	}
	else
	{
		float angulo_cComHorizontal = atan2f(distanciaY, distanciaX);

		alfa = angulo_cComHorizontal + angulo_cComHorizontal;
	}

	float anguloOmbro = alfa + 1 / 2.0 * PI;
	float anguloCotovelo = 1 / 2.0 * PI - beta;

	return Perna(0, anguloOmbro, anguloCotovelo);
}

void moverParaPosicaoNeutra()
{
	//cout << "\n-- Movendo para posicao neutra - clock(): " << clock() << "\n"; // Código comentado (textos)

	for (byte indicePerna = 0; indicePerna < 6; indicePerna++)
	{
		byte indiceServoHorizontal = indicePerna * 3;

		float anguloMinimo = _servo_anguloMinimo[indiceServoHorizontal];
		float anguloMaximo = _servo_anguloMaximo[indiceServoHorizontal];

		float anguloHorizontal = (anguloMaximo - anguloMinimo);

		Perna perna = retornarAngulosPerna(Ponto(xPerto, yBaixo));

		posicionarPerna(indicePerna, anguloHorizontal, perna.anguloOmbro, perna.anguloCotovelo);
	}
}

#pragma region Andar em linha reta

Perna retornarAngulosPerna_Andar(byte indicePerna, byte passo, float direcaoRadianos)
{
	byte indiceServoHorizontal = indicePerna * 3;

	float anguloMinimo = _servo_anguloMinimo[indiceServoHorizontal];
	float anguloMaximo = _servo_anguloMaximo[indiceServoHorizontal];

	float amplitudeHorizontal = (anguloMaximo - anguloMinimo);

	float anguloHorizontal_0 = anguloMinimo;
	float anguloHorizontal_1 = anguloMinimo + amplitudeHorizontal * 1 / 4.0;
	float anguloHorizontal_2 = anguloMinimo + amplitudeHorizontal * 2 / 4.0;
	float anguloHorizontal_3 = anguloMinimo + amplitudeHorizontal * 3 / 4.0;
	float anguloHorizontal_4 = anguloMaximo;


	float amplitudeX = xLonge - xPerto;

	float x_0 = xPerto;
	float x_1 = xPerto + amplitudeX * 1 / 4.0;
	float x_2 = xPerto + amplitudeX * 2 / 4.0;
	float x_3 = xPerto + amplitudeX * 3 / 4.0;
	float x_4 = xLonge;

	// Concatena indicePerna e passo, para tratar ambos num mesmo switch
	// Primeiro caracter equivale ao indice da perna (de 0 até 5)
	// Segundo caracter equivale ao passo (de 0 até 3)
	int indicePerna_passo = indicePerna * 10 + (passo % 6);

	Perna baixo_0 = retornarAngulosPerna(Ponto(x_0, yBaixo));
	Perna baixo_1 = retornarAngulosPerna(Ponto(x_1, yBaixo));
	Perna baixo_2 = retornarAngulosPerna(Ponto(x_2, yBaixo));
	Perna baixo_3 = retornarAngulosPerna(Ponto(x_3, yBaixo));
	Perna baixo_4 = retornarAngulosPerna(Ponto(x_4, yBaixo));

	Perna alto_2 = retornarAngulosPerna(Ponto(x_2, yAlto));

	if (direcaoRadianos == FRENTE)
	{
		switch (indicePerna_passo)
		{
		case 00: return Perna(anguloHorizontal_0, baixo_4.anguloOmbro, baixo_4.anguloCotovelo);
		case 01: return Perna(anguloHorizontal_1, baixo_3.anguloOmbro, baixo_3.anguloCotovelo);
		case 02: return Perna(anguloHorizontal_2, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 03: return Perna(anguloHorizontal_3, baixo_1.anguloOmbro, baixo_1.anguloCotovelo);
		case 04: return Perna(anguloHorizontal_4, baixo_0.anguloOmbro, baixo_0.anguloCotovelo);
		case 05: return Perna(anguloHorizontal_2, alto_2.anguloOmbro, alto_2.anguloCotovelo);

		case 10: return Perna(anguloHorizontal_2, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 11: return Perna(anguloHorizontal_3, baixo_3.anguloOmbro, baixo_3.anguloCotovelo);
		case 12: return Perna(anguloHorizontal_4, baixo_4.anguloOmbro, baixo_4.anguloCotovelo);
		case 13: return Perna(anguloHorizontal_2, alto_2.anguloOmbro, alto_2.anguloCotovelo);
		case 14: return Perna(anguloHorizontal_0, baixo_4.anguloOmbro, baixo_4.anguloCotovelo);
		case 15: return Perna(anguloHorizontal_1, baixo_3.anguloOmbro, baixo_3.anguloCotovelo);

		case 20: return Perna(anguloHorizontal_4, baixo_4.anguloOmbro, baixo_4.anguloCotovelo);
		case 21: return Perna(anguloHorizontal_2, alto_2.anguloOmbro, alto_2.anguloCotovelo);
		case 22: return Perna(anguloHorizontal_0, baixo_0.anguloOmbro, baixo_0.anguloCotovelo);
		case 23: return Perna(anguloHorizontal_1, baixo_1.anguloOmbro, baixo_1.anguloCotovelo);
		case 24: return Perna(anguloHorizontal_2, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 25: return Perna(anguloHorizontal_3, baixo_3.anguloOmbro, baixo_3.anguloCotovelo);

		case 30: return Perna(anguloHorizontal_0, baixo_0.anguloOmbro, baixo_0.anguloCotovelo);
		case 31: return Perna(anguloHorizontal_2, alto_2.anguloOmbro, alto_2.anguloCotovelo);
		case 32: return Perna(anguloHorizontal_4, baixo_4.anguloOmbro, baixo_4.anguloCotovelo);
		case 33: return Perna(anguloHorizontal_3, baixo_3.anguloOmbro, baixo_3.anguloCotovelo);
		case 34: return Perna(anguloHorizontal_2, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 35: return Perna(anguloHorizontal_1, baixo_1.anguloOmbro, baixo_1.anguloCotovelo);

		case 40: return Perna(anguloHorizontal_2, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 41: return Perna(anguloHorizontal_1, baixo_3.anguloOmbro, baixo_3.anguloCotovelo);
		case 42: return Perna(anguloHorizontal_0, baixo_4.anguloOmbro, baixo_4.anguloCotovelo);
		case 43: return Perna(anguloHorizontal_2, alto_2.anguloOmbro, alto_2.anguloCotovelo);
		case 44: return Perna(anguloHorizontal_4, baixo_4.anguloOmbro, baixo_4.anguloCotovelo);
		case 45: return Perna(anguloHorizontal_3, baixo_3.anguloOmbro, baixo_3.anguloCotovelo);

		case 50: return Perna(anguloHorizontal_4, baixo_0.anguloOmbro, baixo_0.anguloCotovelo);
		case 51: return Perna(anguloHorizontal_3, baixo_1.anguloOmbro, baixo_1.anguloCotovelo);
		case 52: return Perna(anguloHorizontal_2, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 53: return Perna(anguloHorizontal_1, baixo_3.anguloOmbro, baixo_3.anguloCotovelo);
		case 54: return Perna(anguloHorizontal_0, baixo_4.anguloOmbro, baixo_4.anguloCotovelo);
		case 55: return Perna(anguloHorizontal_2, alto_2.anguloOmbro, alto_2.anguloCotovelo);
		}
	}

	if (direcaoRadianos == ATRAS)
	{
		switch (indicePerna_passo)
		{
		case 00: return Perna(anguloHorizontal_4, baixo_0.anguloOmbro, baixo_0.anguloCotovelo);
		case 01: return Perna(anguloHorizontal_3, baixo_1.anguloOmbro, baixo_1.anguloCotovelo);
		case 02: return Perna(anguloHorizontal_2, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 03: return Perna(anguloHorizontal_1, baixo_3.anguloOmbro, baixo_3.anguloCotovelo);
		case 04: return Perna(anguloHorizontal_0, baixo_4.anguloOmbro, baixo_4.anguloCotovelo);
		case 05: return Perna(anguloHorizontal_2, alto_2.anguloOmbro, alto_2.anguloCotovelo);

		case 10: return Perna(anguloHorizontal_2, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 11: return Perna(anguloHorizontal_1, baixo_1.anguloOmbro, baixo_1.anguloCotovelo);
		case 12: return Perna(anguloHorizontal_0, baixo_0.anguloOmbro, baixo_0.anguloCotovelo);
		case 13: return Perna(anguloHorizontal_2, alto_2.anguloOmbro, alto_2.anguloCotovelo);
		case 14: return Perna(anguloHorizontal_4, baixo_4.anguloOmbro, baixo_4.anguloCotovelo);
		case 15: return Perna(anguloHorizontal_3, baixo_3.anguloOmbro, baixo_3.anguloCotovelo);

		case 20: return Perna(anguloHorizontal_0, baixo_0.anguloOmbro, baixo_0.anguloCotovelo);
		case 21: return Perna(anguloHorizontal_2, alto_2.anguloOmbro, alto_2.anguloCotovelo);
		case 22: return Perna(anguloHorizontal_4, baixo_4.anguloOmbro, baixo_4.anguloCotovelo);
		case 23: return Perna(anguloHorizontal_3, baixo_3.anguloOmbro, baixo_3.anguloCotovelo);
		case 24: return Perna(anguloHorizontal_2, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 25: return Perna(anguloHorizontal_1, baixo_1.anguloOmbro, baixo_1.anguloCotovelo);

		case 30: return Perna(anguloHorizontal_4, baixo_4.anguloOmbro, baixo_4.anguloCotovelo);
		case 31: return Perna(anguloHorizontal_2, alto_2.anguloOmbro, alto_2.anguloCotovelo);
		case 32: return Perna(anguloHorizontal_0, baixo_0.anguloOmbro, baixo_0.anguloCotovelo);
		case 33: return Perna(anguloHorizontal_1, baixo_1.anguloOmbro, baixo_1.anguloCotovelo);
		case 34: return Perna(anguloHorizontal_2, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 35: return Perna(anguloHorizontal_3, baixo_3.anguloOmbro, baixo_3.anguloCotovelo);

		case 40: return Perna(anguloHorizontal_2, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 41: return Perna(anguloHorizontal_3, baixo_3.anguloOmbro, baixo_3.anguloCotovelo);
		case 42: return Perna(anguloHorizontal_4, baixo_4.anguloOmbro, baixo_4.anguloCotovelo);
		case 43: return Perna(anguloHorizontal_2, alto_2.anguloOmbro, alto_2.anguloCotovelo);
		case 44: return Perna(anguloHorizontal_0, baixo_4.anguloOmbro, baixo_4.anguloCotovelo);
		case 45: return Perna(anguloHorizontal_1, baixo_3.anguloOmbro, baixo_3.anguloCotovelo);

		case 50: return Perna(anguloHorizontal_0, baixo_4.anguloOmbro, baixo_4.anguloCotovelo);
		case 51: return Perna(anguloHorizontal_1, baixo_3.anguloOmbro, baixo_3.anguloCotovelo);
		case 52: return Perna(anguloHorizontal_2, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 53: return Perna(anguloHorizontal_3, baixo_1.anguloOmbro, baixo_1.anguloCotovelo);
		case 54: return Perna(anguloHorizontal_4, baixo_0.anguloOmbro, baixo_0.anguloCotovelo);
		case 55: return Perna(anguloHorizontal_2, alto_2.anguloOmbro, alto_2.anguloCotovelo);

		}
	}

	if (direcaoRadianos == ESQUERDA)
	{
		switch (indicePerna_passo)
		{
		case 00: return Perna(anguloHorizontal_0, baixo_0.anguloOmbro, baixo_0.anguloCotovelo);
		case 01: return Perna(anguloHorizontal_1, baixo_1.anguloOmbro, baixo_1.anguloCotovelo);
		case 02: return Perna(anguloHorizontal_2, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 03: return Perna(anguloHorizontal_3, baixo_3.anguloOmbro, baixo_3.anguloCotovelo);
		case 04: return Perna(anguloHorizontal_4, baixo_4.anguloOmbro, baixo_4.anguloCotovelo);
		case 05: return Perna(anguloHorizontal_2, alto_2.anguloOmbro, alto_2.anguloCotovelo);

		case 10: return Perna(anguloHorizontal_2, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 11: return Perna(anguloHorizontal_2, baixo_3.anguloOmbro, baixo_3.anguloCotovelo);
		case 12: return Perna(anguloHorizontal_2, baixo_4.anguloOmbro, baixo_4.anguloCotovelo);
		case 13: return Perna(anguloHorizontal_2, alto_2.anguloOmbro, alto_2.anguloCotovelo);
		case 14: return Perna(anguloHorizontal_2, baixo_0.anguloOmbro, baixo_0.anguloCotovelo);
		case 15: return Perna(anguloHorizontal_2, baixo_1.anguloOmbro, baixo_1.anguloCotovelo);

		case 20: return Perna(anguloHorizontal_0, baixo_4.anguloOmbro, baixo_4.anguloCotovelo);
		case 21: return Perna(anguloHorizontal_2, alto_2.anguloOmbro, alto_2.anguloCotovelo);
		case 22: return Perna(anguloHorizontal_4, baixo_0.anguloOmbro, baixo_0.anguloCotovelo);
		case 23: return Perna(anguloHorizontal_3, baixo_1.anguloOmbro, baixo_1.anguloCotovelo);
		case 24: return Perna(anguloHorizontal_2, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 25: return Perna(anguloHorizontal_1, baixo_3.anguloOmbro, baixo_3.anguloCotovelo);

		case 30: return Perna(anguloHorizontal_4, baixo_0.anguloOmbro, baixo_0.anguloCotovelo);
		case 31: return Perna(anguloHorizontal_2, alto_2.anguloOmbro, alto_2.anguloCotovelo);
		case 32: return Perna(anguloHorizontal_0, baixo_4.anguloOmbro, baixo_4.anguloCotovelo);
		case 33: return Perna(anguloHorizontal_1, baixo_3.anguloOmbro, baixo_3.anguloCotovelo);
		case 34: return Perna(anguloHorizontal_2, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 35: return Perna(anguloHorizontal_3, baixo_1.anguloOmbro, baixo_1.anguloCotovelo);

		case 40: return Perna(anguloHorizontal_2, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 41: return Perna(anguloHorizontal_2, baixo_1.anguloOmbro, baixo_1.anguloCotovelo);
		case 42: return Perna(anguloHorizontal_2, baixo_0.anguloOmbro, baixo_0.anguloCotovelo);
		case 43: return Perna(anguloHorizontal_2, alto_2.anguloOmbro, alto_2.anguloCotovelo);
		case 44: return Perna(anguloHorizontal_2, baixo_4.anguloOmbro, baixo_4.anguloCotovelo);
		case 45: return Perna(anguloHorizontal_2, baixo_3.anguloOmbro, baixo_3.anguloCotovelo);

		case 50: return Perna(anguloHorizontal_4, baixo_4.anguloOmbro, baixo_4.anguloCotovelo);
		case 51: return Perna(anguloHorizontal_3, baixo_3.anguloOmbro, baixo_3.anguloCotovelo);
		case 52: return Perna(anguloHorizontal_2, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 53: return Perna(anguloHorizontal_1, baixo_1.anguloOmbro, baixo_1.anguloCotovelo);
		case 54: return Perna(anguloHorizontal_0, baixo_0.anguloOmbro, baixo_0.anguloCotovelo);
		case 55: return Perna(anguloHorizontal_2, alto_2.anguloOmbro, alto_2.anguloCotovelo);
		}
	}

	if (direcaoRadianos == DIREITA)
	{
		switch (indicePerna_passo)
		{
		case 00: return Perna(anguloHorizontal_4, baixo_4.anguloOmbro, baixo_4.anguloCotovelo);
		case 01: return Perna(anguloHorizontal_3, baixo_3.anguloOmbro, baixo_3.anguloCotovelo);
		case 02: return Perna(anguloHorizontal_2, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 03: return Perna(anguloHorizontal_1, baixo_1.anguloOmbro, baixo_1.anguloCotovelo);
		case 04: return Perna(anguloHorizontal_0, baixo_0.anguloOmbro, baixo_0.anguloCotovelo);
		case 05: return Perna(anguloHorizontal_2, alto_2.anguloOmbro, alto_2.anguloCotovelo);

		case 10: return Perna(anguloHorizontal_2, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 11: return Perna(anguloHorizontal_2, baixo_1.anguloOmbro, baixo_1.anguloCotovelo);
		case 12: return Perna(anguloHorizontal_2, baixo_0.anguloOmbro, baixo_0.anguloCotovelo);
		case 13: return Perna(anguloHorizontal_2, alto_2.anguloOmbro, alto_2.anguloCotovelo);
		case 14: return Perna(anguloHorizontal_2, baixo_4.anguloOmbro, baixo_4.anguloCotovelo);
		case 15: return Perna(anguloHorizontal_2, baixo_3.anguloOmbro, baixo_3.anguloCotovelo);

		case 20: return Perna(anguloHorizontal_4, baixo_0.anguloOmbro, baixo_0.anguloCotovelo);
		case 21: return Perna(anguloHorizontal_2, alto_2.anguloOmbro, alto_2.anguloCotovelo);
		case 22: return Perna(anguloHorizontal_0, baixo_4.anguloOmbro, baixo_4.anguloCotovelo);
		case 23: return Perna(anguloHorizontal_1, baixo_3.anguloOmbro, baixo_3.anguloCotovelo);
		case 24: return Perna(anguloHorizontal_2, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 25: return Perna(anguloHorizontal_3, baixo_1.anguloOmbro, baixo_1.anguloCotovelo);

		case 30: return Perna(anguloHorizontal_0, baixo_4.anguloOmbro, baixo_4.anguloCotovelo);
		case 31: return Perna(anguloHorizontal_2, alto_2.anguloOmbro, alto_2.anguloCotovelo);
		case 32: return Perna(anguloHorizontal_4, baixo_0.anguloOmbro, baixo_0.anguloCotovelo);
		case 33: return Perna(anguloHorizontal_3, baixo_1.anguloOmbro, baixo_1.anguloCotovelo);
		case 34: return Perna(anguloHorizontal_2, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 35: return Perna(anguloHorizontal_1, baixo_3.anguloOmbro, baixo_3.anguloCotovelo);

		case 40: return Perna(anguloHorizontal_2, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 41: return Perna(anguloHorizontal_2, baixo_3.anguloOmbro, baixo_3.anguloCotovelo);
		case 42: return Perna(anguloHorizontal_2, baixo_4.anguloOmbro, baixo_4.anguloCotovelo);
		case 43: return Perna(anguloHorizontal_2, alto_2.anguloOmbro, alto_2.anguloCotovelo);
		case 44: return Perna(anguloHorizontal_2, baixo_0.anguloOmbro, baixo_0.anguloCotovelo);
		case 45: return Perna(anguloHorizontal_2, baixo_1.anguloOmbro, baixo_1.anguloCotovelo);

		case 50: return Perna(anguloHorizontal_0, baixo_0.anguloOmbro, baixo_0.anguloCotovelo);
		case 51: return Perna(anguloHorizontal_1, baixo_1.anguloOmbro, baixo_1.anguloCotovelo);
		case 52: return Perna(anguloHorizontal_2, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 53: return Perna(anguloHorizontal_3, baixo_3.anguloOmbro, baixo_3.anguloCotovelo);
		case 54: return Perna(anguloHorizontal_4, baixo_4.anguloOmbro, baixo_4.anguloCotovelo);
		case 55: return Perna(anguloHorizontal_2, alto_2.anguloOmbro, alto_2.anguloCotovelo);
		}
	}

	return Perna(0, 0, 0);
}

void irParaPosicao_Andar(byte indicePerna, byte passo, float direcaoRadianos)
{
	byte indiceServoHorizontal = indicePerna * 3;
	byte indiceServoOmbro = indicePerna * 3 + 1;
	byte indiceServoCotovelo = indicePerna * 3 + 2;

	// Cálculo doido para saber o angulo de cada servo da perna
	Perna perna = retornarAngulosPerna_Andar(indicePerna, passo, direcaoRadianos);

	posicionarServo(indiceServoHorizontal, perna.anguloHorizontal);
	posicionarServo(indiceServoOmbro, perna.anguloOmbro);
	posicionarServo(indiceServoCotovelo, perna.anguloCotovelo);
}

void definirPosicao_Andar(byte passo, float direcaoRadianos)
{
	//cout << "-- Andando (Passo " << (passo + 1) << " de " << int(_totalPassos) << ") - clock(): " << clock() << "\n"; // Código comentado (textos)

	// Ir para cada uma das posições, que combinadas resultam no hexapod andando
	for (byte indicePerna = 0; indicePerna < 6; indicePerna++)
	{
		irParaPosicao_Andar(indicePerna, passo, direcaoRadianos);
	}
}

void gerenciarMovimento_Andar(float direcaoRadianos, int tempoDuracao)
{
	switch (_statusMovimento)
	{
	case STATUS_MOVIMENTO_COMPUTAR_INICIO:
		//cout << "\n----------- Andando (" << direcaoRadianos << " rad) -----------\n"; // Código comentado (textos)

		_passo = 0;
		_statusMovimento = STATUS_MOVIMENTO_COMPUTAR_MOVIMENTO;
		_clockInicioMovimento = clock();
		break;

	case STATUS_MOVIMENTO_COMPUTAR_MOVIMENTO:
		_tempoAguardar = tempoDuracao;

		// Caso esteja dentro do tempo tempo limite da execução deste movimento
		if (clock() <= _clockInicioMovimento + tempoDuracao)
		{
			if (clock() > _clockAnterior + _tempoPorPasso)
			{
				// Computar nova posição de destino
				definirPosicao_Andar(_passo % _totalPassos, direcaoRadianos);
				// Guardar clock para contar tempo até o próximo passo
				_clockAnterior = clock();
				_passo++;
				_passo = _passo % _totalPassos;
			}
		}
		// Caso o tempo limite seja excedido, vai para o status de finalização de movimento
		else
		{
			_statusMovimento = STATUS_MOVIMENTO_COMPUTAR_TERMINO;
		}
		break;

	case STATUS_MOVIMENTO_COMPUTAR_TERMINO:
		//cout << "------------------------------------------\n"; // Código comentado (textos)

		// Remover da fila de movimentos item recém-executado
		_filaMovimento.pop_front();
		moverParaPosicaoNeutra();
		_statusMovimento = STATUS_MOVIMENTO_COMPUTAR_INICIO;
		break;
	}
}

void adicionarNaFila_Andar(float direcaoRadianos, int tempoDuracao)
{
	if (_filaMovimento.size() > 0)
	{
		ItemFilaMovimento ultimoItem = _filaMovimento.back();

		if (ultimoItem._nomeMovimento == ANDAR && ultimoItem._direcao == direcaoRadianos)
		{
			tempoDuracao += ultimoItem._tempoDuracao;
			_filaMovimento.pop_back();
		}
	}
	ItemFilaMovimento novoMovimento(ANDAR, tempoDuracao, false, direcaoRadianos);
	_filaMovimento.push_back(novoMovimento);
	//printListaItemFila(_filaMovimento); // Código comentado (fila)
}

#pragma endregion

#pragma region Girar no próprio eixo

Perna retornarAngulosPerna_Girar(byte indicePerna, byte passo, bool sentido)
{
	byte indiceServoHorizontal = indicePerna * 3;

	float anguloMinimo = _servo_anguloMinimo[indiceServoHorizontal];
	float anguloMaximo = _servo_anguloMaximo[indiceServoHorizontal];

	float amplitudeHorizontal = (anguloMaximo - anguloMinimo);

	float anguloHorizontal_0 = anguloMinimo;
	float anguloHorizontal_1 = anguloMinimo + amplitudeHorizontal * 1 / 4.0;
	float anguloHorizontal_2 = anguloMinimo + amplitudeHorizontal * 2 / 4.0;
	float anguloHorizontal_3 = anguloMinimo + amplitudeHorizontal * 3 / 4.0;
	float anguloHorizontal_4 = anguloMaximo;


	float amplitudeX = xLonge - xPerto;

	float x_0 = xPerto;
	float x_1 = xPerto + amplitudeX * 1 / 4.0;
	float x_2 = xPerto + amplitudeX * 2 / 4.0;
	float x_3 = xPerto + amplitudeX * 3 / 4.0;
	float x_4 = xLonge;

	// Concatena indicePerna e passo, para tratar ambos num mesmo switch
	// Primeiro caracter equivale ao indice da perna (de 0 até 5)
	// Segundo caracter equivale ao passo (de 0 até 3)
	int indicePerna_passo = indicePerna * 10 + (passo % 6);

	Perna baixo_0 = retornarAngulosPerna(Ponto(x_0, yBaixo));
	Perna baixo_1 = retornarAngulosPerna(Ponto(x_1, yBaixo));
	Perna baixo_2 = retornarAngulosPerna(Ponto(x_2, yBaixo));
	Perna baixo_3 = retornarAngulosPerna(Ponto(x_3, yBaixo));
	Perna baixo_4 = retornarAngulosPerna(Ponto(x_4, yBaixo));

	Perna alto_2 = retornarAngulosPerna(Ponto(x_2, yAlto));

	if (sentido == HORARIO)
	{
		switch (indicePerna_passo)
		{
		case 00: return Perna(anguloHorizontal_4, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 01: return Perna(anguloHorizontal_3, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 02: return Perna(anguloHorizontal_2, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 03: return Perna(anguloHorizontal_1, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 04: return Perna(anguloHorizontal_0, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 05: return Perna(anguloHorizontal_2, alto_2.anguloOmbro, alto_2.anguloCotovelo);

		case 10: return Perna(anguloHorizontal_2, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 11: return Perna(anguloHorizontal_1, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 12: return Perna(anguloHorizontal_0, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 13: return Perna(anguloHorizontal_2, alto_2.anguloOmbro, alto_2.anguloCotovelo);
		case 14: return Perna(anguloHorizontal_4, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 15: return Perna(anguloHorizontal_3, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);

		case 20: return Perna(anguloHorizontal_0, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 21: return Perna(anguloHorizontal_2, alto_2.anguloOmbro, alto_2.anguloCotovelo);
		case 22: return Perna(anguloHorizontal_4, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 23: return Perna(anguloHorizontal_3, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 24: return Perna(anguloHorizontal_2, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 25: return Perna(anguloHorizontal_1, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);

		case 30: return Perna(anguloHorizontal_0, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 31: return Perna(anguloHorizontal_2, alto_2.anguloOmbro, alto_2.anguloCotovelo);
		case 32: return Perna(anguloHorizontal_4, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 33: return Perna(anguloHorizontal_3, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 34: return Perna(anguloHorizontal_2, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 35: return Perna(anguloHorizontal_1, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);

		case 40: return Perna(anguloHorizontal_2, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 41: return Perna(anguloHorizontal_1, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 42: return Perna(anguloHorizontal_0, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 43: return Perna(anguloHorizontal_2, alto_2.anguloOmbro, alto_2.anguloCotovelo);
		case 44: return Perna(anguloHorizontal_4, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 45: return Perna(anguloHorizontal_3, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);

		case 50: return Perna(anguloHorizontal_4, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 51: return Perna(anguloHorizontal_3, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 52: return Perna(anguloHorizontal_2, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 53: return Perna(anguloHorizontal_1, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 54: return Perna(anguloHorizontal_0, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 55: return Perna(anguloHorizontal_2, alto_2.anguloOmbro, alto_2.anguloCotovelo);
		}
	}

	if (sentido == ANTIHORARIO)
	{
		switch (indicePerna_passo)
		{
		case 00: return Perna(anguloHorizontal_0, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 01: return Perna(anguloHorizontal_1, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 02: return Perna(anguloHorizontal_2, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 03: return Perna(anguloHorizontal_3, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 04: return Perna(anguloHorizontal_4, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 05: return Perna(anguloHorizontal_2, alto_2.anguloOmbro, alto_2.anguloCotovelo);

		case 10: return Perna(anguloHorizontal_2, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 11: return Perna(anguloHorizontal_3, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 12: return Perna(anguloHorizontal_4, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 13: return Perna(anguloHorizontal_2, alto_2.anguloOmbro, alto_2.anguloCotovelo);
		case 14: return Perna(anguloHorizontal_0, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 15: return Perna(anguloHorizontal_1, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);

		case 20: return Perna(anguloHorizontal_4, baixo_2.anguloOmbro, baixo_4.anguloCotovelo);
		case 21: return Perna(anguloHorizontal_2, alto_2.anguloOmbro, alto_2.anguloCotovelo);
		case 22: return Perna(anguloHorizontal_0, baixo_2.anguloOmbro, baixo_0.anguloCotovelo);
		case 23: return Perna(anguloHorizontal_1, baixo_2.anguloOmbro, baixo_1.anguloCotovelo);
		case 24: return Perna(anguloHorizontal_2, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 25: return Perna(anguloHorizontal_3, baixo_2.anguloOmbro, baixo_3.anguloCotovelo);

		case 30: return Perna(anguloHorizontal_4, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 31: return Perna(anguloHorizontal_2, alto_2.anguloOmbro, alto_2.anguloCotovelo);
		case 32: return Perna(anguloHorizontal_0, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 33: return Perna(anguloHorizontal_1, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 34: return Perna(anguloHorizontal_2, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 35: return Perna(anguloHorizontal_3, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);

		case 40: return Perna(anguloHorizontal_2, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 41: return Perna(anguloHorizontal_3, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 42: return Perna(anguloHorizontal_4, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 43: return Perna(anguloHorizontal_2, alto_2.anguloOmbro, alto_2.anguloCotovelo);
		case 44: return Perna(anguloHorizontal_0, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 45: return Perna(anguloHorizontal_1, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);

		case 50: return Perna(anguloHorizontal_0, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 51: return Perna(anguloHorizontal_1, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 52: return Perna(anguloHorizontal_2, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 53: return Perna(anguloHorizontal_3, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 54: return Perna(anguloHorizontal_4, baixo_2.anguloOmbro, baixo_2.anguloCotovelo);
		case 55: return Perna(anguloHorizontal_2, alto_2.anguloOmbro, alto_2.anguloCotovelo);

		}
	}

	return Perna(0, 0, 0);
}

void irParaPosicao_Girar(byte indicePerna, byte passo, bool sentido)
{
	byte indiceServoHorizontal = indicePerna * 3;
	byte indiceServoOmbro = indicePerna * 3 + 1;
	byte indiceServoCotovelo = indicePerna * 3 + 2;

	// Cálculo doido para saber o angulo de cada servo da perna
	Perna perna = retornarAngulosPerna_Girar(indicePerna, passo, sentido);

	posicionarServo(indiceServoHorizontal, perna.anguloHorizontal);
	posicionarServo(indiceServoOmbro, perna.anguloOmbro);
	posicionarServo(indiceServoCotovelo, perna.anguloCotovelo);

}

void definirPosicao_Girar(byte passo, bool sentido)
{
	//cout << "-- Girando (Passo " << (passo + 1) << " de " << int(_totalPassos) << ") - clock(): " << clock() << "\n"; // Código comentado (textos)

	// Ir para cada uma das posições, que combinadas resultam no hexapod girando
	for (byte indicePerna = 0; indicePerna < 6; indicePerna++)
	{
		irParaPosicao_Girar(indicePerna, passo, sentido);
	}
}

void gerenciarMovimento_Girar(bool sentido, int tempoDuracao)
{
	switch (_statusMovimento)
	{
	case STATUS_MOVIMENTO_COMPUTAR_INICIO:

		if (sentido == HORARIO)
		{
			//cout << "\n------- Girando (sentido horario) --------\n"; // Código comentado (textos)
		}
		else
		{
			//cout << "\n----- Girando (sentido anti-horario) -----\n"; // Código comentado (textos)
		}

		_passo = 0;
		_statusMovimento = STATUS_MOVIMENTO_COMPUTAR_MOVIMENTO;
		_clockInicioMovimento = clock();
		break;

	case STATUS_MOVIMENTO_COMPUTAR_MOVIMENTO:
		_tempoAguardar = tempoDuracao;

		if (clock() <= _clockInicioMovimento + tempoDuracao)
		{
			if (clock() > _clockAnterior + _tempoPorPasso)
			{
				// Computar nova posição de destino
				definirPosicao_Girar(_passo % _totalPassos, sentido);
				// Guardar clock para contar tempo até o próximo passo
				_clockAnterior = clock();
				_passo++;
				_passo = _passo % _totalPassos;
			}
		}
		// Caso o tempo limite seja excedido, vai para o status de finalização de movimento
		else
		{
			_statusMovimento = STATUS_MOVIMENTO_COMPUTAR_TERMINO;
		}
		break;

	case STATUS_MOVIMENTO_COMPUTAR_TERMINO:
		//cout << "------------------------------------------\n"; // Código comentado (textos)

		// Remover da fila de movimentos item recém-executado
		_filaMovimento.pop_front();
		moverParaPosicaoNeutra();
		_statusMovimento = STATUS_MOVIMENTO_COMPUTAR_INICIO;
		break;
	}
}

void adicionarNaFila_Girar(bool sentido, int tempoDuracao)
{
	if (_filaMovimento.size() > 0)
	{
		ItemFilaMovimento ultimoItem = _filaMovimento.back();

		if (ultimoItem._nomeMovimento == GIRAR && ultimoItem._sentido == sentido)
		{
			tempoDuracao += ultimoItem._tempoDuracao;
			_filaMovimento.pop_back();
		}
	}
	ItemFilaMovimento novoMovimento(GIRAR, tempoDuracao, sentido, 0);
	_filaMovimento.push_back(novoMovimento);
	//printListaItemFila(_filaMovimento); // Código comentado (fila)
}

#pragma endregion

void atualizarPosicaoHexapod()
{
	bool anguloAtualDiferenteAnguloDestino = false;

	// Caso algum motor não esteja na posição de destino
	for (byte indiceServo = 0; indiceServo < 18; indiceServo++)
	{
		if (_servo_anguloAtual[indiceServo] != _servo_anguloDestino[indiceServo])
		{
			moverServosParaPosicaoDestinoGradativamente();
			return;
		}
	}
	
	if (_filaMovimento.size() > 0)
	{
		ItemFilaMovimento primeiroItem = _filaMovimento.front();

		switch (primeiroItem._nomeMovimento)
		{
		case ANDAR:
			gerenciarMovimento_Andar(primeiroItem._direcao, primeiroItem._tempoDuracao);
			break;
		case GIRAR:
			gerenciarMovimento_Girar(primeiroItem._sentido, primeiroItem._tempoDuracao);
			break;
		default:
			cout << "Erro: Movimento não encontrado/n";
			break;
		}
	}
}

