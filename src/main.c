
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

/* ---------------- Parâmetros (altere para as Partes 2 e 3) ---------------- */
#define T_PADEIRO_MS        1000  /* Parte 1: 1000 | Parte 2.1: 500  | Parte 2.2: 2000 */
#define T_CLIENTE_MS        1500  /* Parte 1: 1500 | Parte 2.1: 2000 | Parte 2.2: 500  */

/* No Zephyr: NÚMERO MENOR = PRIORIDADE MAIOR */
#define PRIO_PADEIRO        5     /* Parte 3.1: 4 (padeiro maior) | Parte 3.2: 6 (cliente maior) */
#define PRIO_CLIENTE        5

#define CAPACIDADE_VITRINE  10
#define STACK_SIZE          1024

/* 1 = abre uma "janela" entre ler e gravar o saldo, para forçar a race condition */
#define SIMULAR_RACE        0
/* ------------------------------------------------------------------------- */

/* Recurso compartilhado SEM proteção */
static volatile int saldo_vitrine = 0;

static inline void vitrine_incrementa(void)
{
#if SIMULAR_RACE
	/* saldo_vitrine++ é na prática: LOAD -> ADD -> STORE.
	 * Aqui separamos as etapas e alargamos a janela entre elas. */
	int tmp = saldo_vitrine;
	k_busy_wait(3000);            /* 3 ms: o time slice pode preemptar aqui */
	saldo_vitrine = tmp + 1;
#else
	saldo_vitrine++;
#endif
}

static inline void vitrine_decrementa(void)
{
#if SIMULAR_RACE
	int tmp = saldo_vitrine;
	k_busy_wait(3000);
	saldo_vitrine = tmp - 1;
#else
	saldo_vitrine--;
#endif
}

/* Thread 1: padeiro */
static void padeiro_thread(void *p1, void *p2, void *p3)
{
	int n = 0;

	while (1) {
		k_msleep(T_PADEIRO_MS);

		n++;
		vitrine_incrementa();     /* produz sem verificar se há espaço */
		printk("PADEIRO: Pao #%d pronto (Saldo vitrine = %d)\n", n, saldo_vitrine);

		if (saldo_vitrine > CAPACIDADE_VITRINE) {
			printk("  [!] Vitrine estourou: %d paes (cabem %d)\n",
			       saldo_vitrine, CAPACIDADE_VITRINE);
		}
	}
}

/* Thread 2: cliente */
static void cliente_thread(void *p1, void *p2, void *p3)
{
	int n = 0;

	while (1) {
		k_msleep(T_CLIENTE_MS);

		n++;
		vitrine_decrementa();     /* consome sem verificar se há pão */
		printk("CLIENTE: Peguei pao #%d (Saldo vitrine = %d)\n", n, saldo_vitrine);

		if (saldo_vitrine < 0) {
			printk("  [!] Saldo negativo: cliente pegou pao que nao existe!\n");
		}
	}
}

K_THREAD_DEFINE(padeiro_id, STACK_SIZE, padeiro_thread, NULL, NULL, NULL,
		PRIO_PADEIRO, 0, 0);
K_THREAD_DEFINE(cliente_id, STACK_SIZE, cliente_thread, NULL, NULL, NULL,
		PRIO_CLIENTE, 0, 0);

int main(void)
{
	printk("Padaria iniciada (sem sincronizacao)\n");
	return 0;
}