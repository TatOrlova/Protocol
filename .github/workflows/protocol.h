#ifndef PROTOCOL_H
#define PROTOCOL_H


//Описание протокола:

//Преамбула 4 байта 0x01 0x02 0x03 0x04
//Длина сообщения 1 байт
//Адрес источника 1 байт
//Адрес приёмника 1 байт
//Данные N байт
//CRC - XOR всех байт сообщения включая преамбулу

//максимальное время приёма сообщения 1 сек. ЗАЧЕМ



// Адрес устройства
#define ADDRESS 50

// Время в которое должна приходить одна посылка в мс
#define MAX_TIME_ONE_PACKAGE 1000 

// Размер преамбулы preamble
#define PREAMBLE_SIZE 4

// Сама преамбула
uint8_t preamble_arr[PREAMBLE_SIZE] = {0x01, 0x02, 0x03, 0x04};

// Размер данных максимальный
#define DATA_SIZE_MAХ 256


// Для перечислений состояний приема и записи
enum {
	NO_RECEPTION = 0,										// Нет приема
	PREAMBLE_RECEPTION,										// Приходит преамбула
	SIZE_RECEPTION,											// Длинна данных
	ADDRESS_TRANSMITTER,									// Адрес источника
	ADDRESS_RECEIVER,  										// Адрес приемника
	DATA_RECEPTION,											// Приходят данные
	CHECK_SUM_RECEPTION,									// Приходит контрольная сумма
	SAVE_TO_BUFF_LOCK										// Данные переписываются в буфер для чего-то
};


// Для перечислений состояний передачи данных
enum {
	NO_TRANSFER = 0,										// Нет передачи данных
	FILLING_BUFF_TRANSFER, 									// Наполнение буфера 
	TRANSFER_BUFF											// Передача буфера
};

// Состояние передачи и приема
uint8_t transfer_status = NO_TRANSFER;
uint8_t reception_status = NO_RECEPTION;

// Буферы для отправки - приема
uint8_t transfer_data_buff[DATA_SIZE_MAХ];
uint8_t reception_data_buff[DATA_SIZE_MAХ];


// Длинна буферов
uint8_t transfer_data_size;
uint8_t reception_data_size;

// Счетчики для буферов
uint8_t transfer_data_count = 0;
uint8_t reception_data_count = 0;

// Источник данных
uint8_t transmitter_address;

// Функция вычисления CRC с известной преамбулой
uint8_t CRC_get(uint8_t * data_buff,  uint8_t data_buff_size);


// Функции для передачи

// Функция возвращает состояние передачи
uint8_t transfer_status_get();

// Функция заполнения буфера
transfer_set_data_buff_and_start_transfer(uint8_t * data_buff,  uint8_t data_buff_size);

// Функция начала отправки. Перед отправкой нужно проверить, что предыдущая посылка полнностью отправлена
transfer_start();

// Функция продолжения отправки. Навешивается на прерывание по окончанию отправки предыдущего байта
transfer_continuation();



// Функции для приема

// Функция возвращает состояние приема
uint8_t reception_status_get();

// ОСНОВНАЯ ФУНКЦИЯ ПРИЕМА навешивается на прерывание по приему одного байта
reception_byte(uint8_t data_byte);

// Функция возвращает буфер, который приняли. И разрешает дальнейший прием.
// Возвращает длинну буфера. Если буфер меньше чем нужно запишет все что влезет.
uint8_t reception_data_buff_return(uint8_t * data_buff, uint8_t data_buff_size);

// Функция возвращает адрес передатчика
uint8_t reception_transmitter_get();

// Функция сброса приема. Навсякий случай. Например не нужны данные от данного источника.
reception_reset();

// Сразу распишу тут прием, чтобы не городить другой файл
reception_byte(uint8_t data_byte)
{
	switch(reception_status)
		{
			case NO_RECEPTION:
			{
				// проверяем пришло ли первый байт преамбулы
				if(data_byte == preamble_arr[0])
				{
					reception_data_count = 1;
					reception_status = PREAMBLE_RECEPTION;
				}			
			}
				break;
				
			// Приходит преамбула и адреса и длина
			case PREAMBLE_RECEPTION:
			{		
				if(reception_data_count < PREAMBLE_SIZE)
				{
					// Проверяем что приамбула верна
					if(data_byte != preamble_arr[reception_data_count++])
					{
						// Если преамбула не верна сбрасываем прием и выставляем паузу
						reception_status = NO_RECEPTION;
					}
				}
				
				if(reception_data_count == PREAMBLE_SIZE)
					// Если преамбула пришла и верна следующим будет длинна 
						reception_status = SIZE_RECEPTION;
			}
				break;
				
			// Приходит длинна данных
			case SIZE_RECEPTION:
			{
				reception_data_size = data_byte;
				reception_status = ADDRESS_TRANSMITTER;
			}
				break;
				
			// Приходит адрес источника
			case ADDRESS_TRANSMITTER:
			{
				transmitter_address = data_byte;
				reception_status = ADDRESS_RECEIVER;
			}
				break;	
				
			// Приходит адрес приемника
			case ADDRESS_RECEIVER:
			{
				if(data_byte != ADDRESS)
				{
					// Если не адрес устройства сбрасываем прием
					reception_status = NO_RECEPTION;
				}
				else
				{
					reception_status = DATA_RECEPTION;
					reception_data_count = 0;
				}
			}
				break;	
				
				
			// Приходят данные
			case DATA_RECEPTION:
			{
				reception_data_buff[reception_data_count++] = data;
				// если пришли все данные
				if(reception_data_count == reception_data_size)
				{
					reception_status = CHECK_SUM_RECEPTION;
					reception_data_count = 0;
				}
			}
				break;	
				
			// Пришла CRC
			case CHECK_SUM_RECEPTION:
			{
				if(CRC_get(reception_data_buff, reception_data_size) == data)
				{
					reception_status = SAVE_TO_BUFF_LOCK;
				}
				// Если CRC не верна сбрасываем прием
				else
				{
					reception_status = NO_RECEPTION;
				}
			}
				break;		
				
			case SAVE_TO_BUFF_LOCK:	
			{
					// Ждем пока считают буфер
			}
				break;
		}
	
}



// Функция возвращает буфер, который приняли. И разрешает дальнейший прием.
// Возвращает длинну буфера. Если буфер меньше чем нужно запишет все что влезет.
uint8_t reception_data_buff_return(uint8_t * data_buff, uint8_t data_buff_size)
{
	uint8_t data_count;
	for(data_count = 0; ((data_count < reception_data_size) && (data_count < data_buff_size)); data_count++)
	{
		data_buff[data_count] = reception_data_buff[data_count];
	}
	reception_status = NO_RECEPTION;
	return data_count;
}




#endif // PROTOCOL_H


