
/*
Relação entre os indices (de 0 a 17) e os servos de cada perna do hexapod
0	-->	PERNA_DIREITA_FRONTAL_SERVO_HORIZONTAL
1	-->	PERNA_DIREITA_FRONTAL_SERVO_OMBRO
2	-->	PERNA_DIREITA_FRONTAL_SERVO_COTOVELO
3	-->	PERNA_DIREITA_CENTRAL_SERVO_HORIZONTAL
4	-->	PERNA_DIREITA_CENTRAL_SERVO_OMBRO
5	-->	PERNA_DIREITA_CENTRAL_SERVO_COTOVELO
6	-->	PERNA_DIREITA_TRASEIRA_SERVO_HORIZONTAL
7	-->	PERNA_DIREITA_TRASEIRA_SERVO_OMBRO
8	-->	PERNA_DIREITA_TRASEIRA_SERVO_COTOVELO
9	-->	PERNA_ESQUERDA_FRONTAL_SERVO_HORIZONTAL
10	-->	PERNA_ESQUERDA_FRONTAL_SERVO_OMBRO
11	-->	PERNA_ESQUERDA_FRONTAL_SERVO_COTOVELO
12	-->	PERNA_ESQUERDA_CENTRAL_SERVO_HORIZONTAL
13	-->	PERNA_ESQUERDA_CENTRAL_SERVO_OMBRO
14	-->	PERNA_ESQUERDA_CENTRAL_SERVO_COTOVELO
15	-->	PERNA_ESQUERDA_TRASEIRA_SERVO_HORIZONTAL
16	-->	PERNA_ESQUERDA_TRASEIRA_SERVO_OMBRO
17	-->	PERNA_ESQUERDA_TRASEIRA_SERVO_COTOVELO

xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
x.............................................x
x.............................................x
x...........(3).................(0)...........x
x............\ \.............../ /............x
x.............\ \.....90°...../ /.............x
x..............\ \___________/ /..............x
x..........45°../o           o\..45°..........x
x......._______/               \_______.......x
x....(4)_______|o      x      o|_______(1)....x
x..............\               /..............x
x..........45°..\o___________o/..45°..........x
x............../ /...........\ \..............x
x............./ /.....90°.....\ \.............x
x............/ /...............\ \............x
x...........(5).................(2)...........x
x.............................................x
x.............................................x
xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

*/

#pragma region Variáveis Globais

const float PI = 3.14159265358979323846;

// pinos onde os servos estão ligados
const byte _servo_pino[18] = {
	1, 2, 3,
	4, 5, 6,
	7, 8, 9,
	10, 11, 12,
	13, 14, 15,
	16, 17, 18
};

// angulo atual dos servos
float _servo_anguloAtual[18] = {
	1 / 2.0 * PI, 1 / 3.0 * PI, 5 / 6.0 * PI,
	1 / 2.0 * PI, 1 / 3.0 * PI, 5 / 6.0 * PI,
	1 / 2.0 * PI, 1 / 3.0 * PI, 5 / 6.0 * PI,
	1 / 2.0 * PI, 1 / 3.0 * PI, 5 / 6.0 * PI,
	1 / 2.0 * PI, 1 / 3.0 * PI, 5 / 6.0 * PI,
	1 / 2.0 * PI, 1 / 3.0 * PI, 5 / 6.0 * PI
};

// angulo destino dos servos
float _servo_anguloDestino[18] = {
	1 / 2.0 * PI, 1 / 3.0 * PI, 5 / 6.0 * PI,
	1 / 2.0 * PI, 1 / 3.0 * PI, 5 / 6.0 * PI,
	1 / 2.0 * PI, 1 / 3.0 * PI, 5 / 6.0 * PI,
	1 / 2.0 * PI, 1 / 3.0 * PI, 5 / 6.0 * PI,
	1 / 2.0 * PI, 1 / 3.0 * PI, 5 / 6.0 * PI,
	1 / 2.0 * PI, 1 / 3.0 * PI, 5 / 6.0 * PI
};

// angulo minimo dos servos
float _servo_anguloMinimo[18] = {
	2 / 8.0 * PI, 0, 0, // pernas 0 tem liberdade maior no servoHorizontal, pois existe um espaço maior até a perna mais próxima (perna 3)
	3 / 8.0 * PI, 0, 0,
	3 / 8.0 * PI, 0, 0,
	3 / 8.0 * PI, 0, 0,
	3 / 8.0 * PI, 0, 0,
	2 / 8.0 * PI, 0, 0 //  pernas 5 tem liberdade maior no servoHorizontal, pois existe um espaço maior até a perna mais próxima (perna 2)
};

// angulo maximo dos servos
float _servo_anguloMaximo[18] = {
	5 / 8.0 * PI, PI, PI,
	5 / 8.0 * PI, PI, PI,
	5 / 8.0 * PI, PI, PI,
	5 / 8.0 * PI, PI, PI,
	5 / 8.0 * PI, PI, PI,
	5 / 8.0 * PI, PI, PI
};

float xPerto = 16;
float xLonge = 20;

float yAlto = 0;
float yBaixo = -5;

Ponto pontoPertoBaixo(xPerto, yBaixo);
Ponto pontoLongeBaixo(xLonge, yBaixo);
Ponto pontoPertoAlto(xPerto, yAlto);
Ponto pontoLongeAlto(xLonge, yAlto);

#pragma endregion

#pragma region Funções auxiliares

float limitarRangeAngulo(byte indiceServo, float angulo)
{
	float anguloMinimo = _servo_anguloMinimo[indiceServo];
	float anguloMaximo = _servo_anguloMaximo[indiceServo];

	if (angulo < anguloMinimo)
	{
		return anguloMinimo;
	}

	if (angulo > anguloMaximo)
	{
		return anguloMaximo;
	}

	return angulo;
}

void posicionarServo(byte indiceServo, float anguloDestino)
{
	_servo_anguloDestino[indiceServo] = anguloDestino;
}

void posicionarPerna(byte indicePerna, float servoHorizontal, float servoOmbro, float servoCotovelo)
{
	/*
	posicionarPerna(indicePerna, anguloPosicaoHorizontal, anguloPosicaoOmbro, anguloPosicaoCotovelo)
	 indicePerna				: de 0 a 5
	 anguloPosicaoHorizontal	: 0 --> esquerda	e 180 --> direita
	 anguloPosicaoOmbro			: 0 --> baixo		e 180 --> alto
	 anguloPosicaoCotovelo		: 0 --> baixo		e 180 --> alto

	indicePerna = 0 --> Perna direita frontal
	indicePerna = 1 --> Perna direita central
	indicePerna = 2 --> Perna direita traseira
	indicePerna = 3 --> Perna esquerda frontal
	indicePerna = 4 --> Perna esquerda central
	indicePerna = 5 --> Perna esquerda traseira
	*/

	byte indiceServoHorizontal = indicePerna * 3;
	byte indiceServoOmbro = indicePerna * 3 + 1;
	byte indiceServoCotovelo = indicePerna * 3 + 2;

	posicionarServo(indiceServoHorizontal, servoHorizontal);
	posicionarServo(indiceServoOmbro, servoOmbro);
	posicionarServo(indiceServoCotovelo, servoCotovelo);
}

void moverServosParaPosicaoDestinoGradativamente()
{
	for (byte indiceServo = 0; indiceServo < 18; indiceServo++)
	{
		// Mantém os angulos 'atual' e 'destino' dentro dos devidos ranges possíveis
		_servo_anguloAtual[indiceServo] = limitarRangeAngulo(indiceServo, _servo_anguloAtual[indiceServo]);
		_servo_anguloDestino[indiceServo] = limitarRangeAngulo(indiceServo, _servo_anguloDestino[indiceServo]);

		float diffAngulos = abs(_servo_anguloDestino[indiceServo] - _servo_anguloAtual[indiceServo]);

		// 0.1 rad = 5.72958 graus

		float incremento = 0.02;

		// Caso  exista uma diferença muito grande entre os angulos, move aos poucos
		if (diffAngulos > incremento)
		{
			// Caso o anguloDestino seja maior, anguloAtual vai incrementando aos poucos
			if (_servo_anguloDestino[indiceServo] > _servo_anguloAtual[indiceServo])
			{
				_servo_anguloAtual[indiceServo] += incremento;
			}
			// Caso o anguloDestino seja menor, anguloAtual vai decrementando aos poucos
			else if (_servo_anguloDestino[indiceServo] < _servo_anguloAtual[indiceServo])
			{
				_servo_anguloAtual[indiceServo] -= incremento;
			}

			// Confirma que o anguloAtual está no range pre-definido
			_servo_anguloAtual[indiceServo] = limitarRangeAngulo(indiceServo, _servo_anguloAtual[indiceServo]);

			// Move o servo para a posição do anguloAtual
			//analogWrite(indiceServo, _servo_anguloAtual[indiceServo]) // Código comentado
		}
		// Caso a diferença entre os angulos seja baixa, move direto para o anguloDestino
		else
		{
			// Confirma que o anguloDestino está no range pre-definido
			_servo_anguloDestino[indiceServo] = limitarRangeAngulo(indiceServo, _servo_anguloDestino[indiceServo]);

			// atualiza o anguloAtual com o valor do anguloDestino
			_servo_anguloAtual[indiceServo] = _servo_anguloDestino[indiceServo];

			// Move o servo para a posição do anguloAtual
			//analogWrite(indiceServo, _servo_anguloDestino[indiceServo]) // Código comentado
		}
	}

	for (byte indicePerna = 0; indicePerna < 1; indicePerna++)
	{
		cout << "Perna " << int(indicePerna) << " - "; // Código comentado (servos)
		cout << "Horizontal: " << _servo_anguloAtual[indicePerna * 3] << '\t'; // Código comentado (servos)
		cout << "Ombro: " << _servo_anguloAtual[indicePerna * 3 + 1] << '\t'; // Código comentado (servos)
		cout << "Cotovelo: " << _servo_anguloAtual[indicePerna * 3 + 2] << '\t'; // Código comentado (servos)
		cout << '\n'; // Código comentado (servos)
	}
}

#pragma endregion
