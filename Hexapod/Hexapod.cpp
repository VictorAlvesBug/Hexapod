// Hexapod.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "Botoes.h"

int _contadorAnterior;

const char ESC = 27;

/*
PONTEIROS:

// O ponteiro armazena o endere�o de mem�ria de determinada vari�vel.
// O tipo do ponteiro tem que ser do mesmo tipo da vari�vel que est� no endere�o de mem�ria apontado.

// Declarando uma vari�vel comum.
int numero = 10;

// Declarando um ponteiro.
// O prefixo "*" no nome da vari�vel, indica que ela � um ponteiro.
int *ponteiroNumero;

// Armazenando no ponteiro o endere�o de mem�ria da vari�vel "numero".
// O prefixo & no nome da vari�vel, indica que est� sendo retornado o endere�o de mem�ria dela.
ponteiroNumero = &numero;

// Imprimindo o endere�o da vari�vel "numero" a partir do ponteiro "ponteiroNumero".
cout << ponteiroNumero;

// Imprimindo o valor da vari�vel "numero" a partir do ponteiro "ponteiroNumero" (usando "*" antes do nome).
cout << *ponteiroNumero;

// Alterando o valor da vari�vel "numero" a partir do ponteiro "ponteiroNumero" (usando "*" antes do nome).
*ponteiroNumero = 15;

*/

#pragma region Fun��es auxiliares

void setup()
{
	//cout << "Setup();\n"; // C�digo comentado
}

void loop()
{
	if (verificarPressionouTeclaValida())
	{
		computarAcaoTeclaPressionada();
	}

	atualizarPosicaoHexapod();
}

#pragma endregion


#pragma region Fun��o principal

int main()
{
	setup();

	_contadorAnterior = -1;

	while (true)
	{
		int contador = clock() / _delay;

		if (contador > _contadorAnterior)
		{
			loop();
			_contadorAnterior = contador;
		}

		if (GetKeyState(ESC) & 0x8000)
		{
			return 0;
		}
	}
}

#pragma endregion

