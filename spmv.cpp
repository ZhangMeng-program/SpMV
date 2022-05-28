#include "csr_formatter.h"
#include "spm_allocated.h"
#include "gem5/m5ops.h"

// #define WARMUP_NUM 2
// #define EXECUTE_NUM 5

int WARMUP_NUM = 0;
int EXECUTE_NUM = 1;

void SpMV(CSR &csr, double *x, double *y)
{
	for (int i = 0; i < csr.row_ptr.size() - 1; i++)
	{
		for (int j = (csr.row_ptr[i] - 1); j < (csr.row_ptr[i + 1] - 1); j++)
		{
			y[i] += csr.val[j] * x[csr.col_ind[j] - 1];
		}
	}
}

void execute_SpMV(CSR &csr, int position)
{

	extern int WARMUP_NUM;
	extern int EXECUTE_NUM;

	// printf("WARMUP_NUM: %d\n", WARMUP_NUM);
	// printf("EXECUTE_NUM: %d\n", EXECUTE_NUM);

	int col = csr.col;
	double *x;
	double y[col] = {0};

	if (position == 0) // allocate in SPM
	{
		printf("Executing SpMV with SPM...\n");
		uint64_t offset = 6442450944;
		x = (double *)spm_mem_alloc(sizeof(double) * col, offset);
	}
	else if (position == 1) // allocate in memory
	{
		printf("Executing SpMV without SPM...\n");
		x = (double *)malloc(sizeof(double) * col);
	}
	else
	{
		printf("Invalid position: %d\n", position);
		exit(1);
	}

	if (x == NULL)
	{
		printf("spm_mem_alloc or malloc failed.\n");
		exit(1);
	}

	for (int i = 0; i < col; i++)
		x[i] = 1.0;

	for (int i = 0; i < WARMUP_NUM + EXECUTE_NUM; i++)
	{
		if (i == WARMUP_NUM)
			m5_reset_stats(0, 0);
		SpMV(csr, x, y);
	}
	m5_dump_stats(0, 0);
	printf("SpMV execution finish.\n\n");
	free(x);
}

int main(int argc, char *argv[])
{

	int position; // SPM 0 or Memory 1
	extern int WARMUP_NUM;
	extern int EXECUTE_NUM;

	if (argc != 5)
	{
		printf("Missing parameters.\n");
		exit(1);
	}

	position = atoi(argv[1]);
	CSR asym = assemble_csr_matrix(argv[2]);
	WARMUP_NUM = atoi(argv[3]);
	EXECUTE_NUM = atoi(argv[4]);

	printf("matrix: %s\n", argv[2]);
	printf("rows: %d\tcols: %d\tnnz: %d\n", asym.row, asym.col, asym.nnz);
	printf("WARMUP_NUM: %d\tEXECUTE_NUM: %d\n", WARMUP_NUM, EXECUTE_NUM);

	switch (position)
	{
	case 0:
		execute_SpMV(asym, 0);
		break;
	case 1:
		execute_SpMV(asym, 1);
		break;
	case 2:
		execute_SpMV(asym, 0);
		execute_SpMV(asym, 1);
		break;
	default:
		break;
	}

	return 0;
}
