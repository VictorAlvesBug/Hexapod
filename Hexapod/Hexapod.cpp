// Hexapod.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "Botoes.h"

int _contadorAnterior;

const char ESC = 27;

/*
PONTEIROS:

// O ponteiro armazena o endereço de memória de determinada variável.
// O tipo do ponteiro tem que ser do mesmo tipo da variável que está no endereço de memória apontado.

// Declarando uma variável comum.
int numero = 10;

// Declarando um ponteiro.
// O prefixo "*" no nome da variável, indica que ela é um ponteiro.
int *ponteiroNumero;

// Armazenando no ponteiro o endereço de memória da variável "numero".
// O prefixo & no nome da variável, indica que está sendo retornado o endereço de memória dela.
ponteiroNumero = &numero;

// Imprimindo o endereço da variável "numero" a partir do ponteiro "ponteiroNumero".
cout << ponteiroNumero;

// Imprimindo o valor da variável "numero" a partir do ponteiro "ponteiroNumero" (usando "*" antes do nome).
cout << *ponteiroNumero;

// Alterando o valor da variável "numero" a partir do ponteiro "ponteiroNumero" (usando "*" antes do nome).
*ponteiroNumero = 15;

*/

#pragma region Funções auxiliares

void setup()
{
	//cout << "Setup();\n"; // Código comentado
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


#pragma region Função principal

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

