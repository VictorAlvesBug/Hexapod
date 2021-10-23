
#include<iostream>
#include<string>
#include <list>
#include <iterator>
using namespace std;

const byte ANDAR = 1;
const byte GIRAR = 2;

class ItemFilaMovimento
{
public:
	int _clockCadastro;
	int _nomeMovimento;
	int _tempoDuracao;
	bool _sentido;
	float _direcao;

	ItemFilaMovimento(int nomeMovimento, int tempoDuracao, bool sentido, float direcao)
	{
		_clockCadastro = clock();
		_nomeMovimento = nomeMovimento;
		_tempoDuracao = tempoDuracao;
		_sentido = sentido;
		_direcao = direcao;
	}
};

#pragma region Funções auxiliares

void printItemFila(ItemFilaMovimento item)
{
	switch (item._nomeMovimento)
	{
	case ANDAR:
		cout << "Movimento: ANDAR\t";
		cout << "TempoDuracao: " << item._tempoDuracao << "\t";
		cout << "Direcao: " << item._direcao << "\t";
		cout << "ClockCadastro: " << item._clockCadastro << "\t";
		cout << '\n';
		break;

	case GIRAR:
		cout << "Movimento: GIRAR\t";
		cout << "TempoDuracao: " << item._tempoDuracao << "\t";

		if (item._sentido)
		{
			cout << "Sentido: Horario\t";
		}
		else
		{
			cout << "Sentido: Anti-horario\t";
		}

		cout << "ClockCadastro: " << item._clockCadastro << "\t";
		cout << '\n';
		break;
	}


}

void printListaItemFila(list <ItemFilaMovimento> lista)
{
	list <ItemFilaMovimento> ::iterator item;
	for (item = lista.begin(); item != lista.end(); ++item)
	{
		printItemFila(*item);
	}
	cout << '\n';
}

#pragma endregion
