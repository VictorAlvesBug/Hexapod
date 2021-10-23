
#include <Windows.h>
#include "Controlador.h"

const int _delay = 2;

char _teclaAnterior = ' ';
char _teclaPressionada;
char _teclasValidas[] = { 'W', 'A', 'S', 'D', VK_LEFT, VK_RIGHT }; 

bool verificarPressionouTeclaValida()
{
	byte lengthTeclasValidas = sizeof(_teclasValidas) / sizeof(_teclasValidas[0]);

	for (byte indice = 0; indice < lengthTeclasValidas; indice++)
	{
		if (GetKeyState(_teclasValidas[indice]) & 0x8000)
		{
			_teclaAnterior = _teclaPressionada;
			_teclaPressionada = _teclasValidas[indice];
			return true;
		}
	}

	return false;
}

void computarAcaoTeclaPressionada() 
{
	string nomeTecla = { _teclaPressionada };

	if (_teclaPressionada == VK_LEFT)
	{
		nomeTecla = "ARROW_LEFT";
	}
	else if (_teclaPressionada == VK_RIGHT)
	{
		nomeTecla = "ARROW_RIGHT";
	}

	//cout << "Pressionou a tecla '" << nomeTecla << "'\n"; // Código comentado (textos)

	switch (_teclaPressionada)
	{
	case 'W':
		adicionarNaFila_Andar(FRENTE, _delay);
		break;

	case 'A':
		adicionarNaFila_Andar(ESQUERDA, _delay);
		break;

	case 'S':
		adicionarNaFila_Andar(ATRAS, _delay);
		break;

	case 'D':
		adicionarNaFila_Andar(DIREITA, _delay);
		break;

	case VK_LEFT:
		adicionarNaFila_Girar(ANTIHORARIO, _delay);
		break;

	case VK_RIGHT:
		adicionarNaFila_Girar(HORARIO, _delay);
		break;
	}
}