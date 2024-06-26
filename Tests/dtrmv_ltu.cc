#include <blas/dtrmv_ltu.hh>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <iomanip>

using namespace CDC8600;

extern "C" int32_t dtrmv_(char *, char *, char *, int32_t *, double *, int32_t *, double *, int32_t *);

const u32 N = 20;
const double EPSILON = 1e-9;

void test_dtrmv_ltu(int count, bool traceon, i32 n, i32 lda, i32 incx)
{
    reset();

    char uplo = 'L';
    char trans = 'T';
    char diag = 'U';
    u64 total_cycles = 0;
    u64 max_cycles = 0;
    u64 num_procs = 0;

    // tracing = false; if (traceon) tracing = true;

    int32_t nx = 1 + n * abs(incx);

    f64 *A = (f64*)CDC8600::memalloc(n*lda);
    f64 *x = (f64*)CDC8600::memalloc(nx);
    f64 *X = new f64[nx];

    for (int i = 0; i < nx; i++) { X[i] = x[i] = drand48(); }
    for (int i = 0; i < n*lda; i++) { A[i] = drand48(); }

    dtrmv_(&uplo, &trans, &diag, &n, A, &lda, X, &incx);
    CDC8600::BLAS::dtrmv_ltu(n, A, lda, x, incx);

    bool pass = true;
    for (int i = 0; i < nx; i++)
    {
        if(!(abs(X[i] - x[i]) < (min(abs(X[i]), abs(x[i])) + EPSILON) * EPSILON))
        {
            pass = false;
            break;
        }
    }

    delete [] X;

    if (traceon) {
        for (u32 i = 0; i < params::Proc::N; i++) {
            dump(PROC[i].trace);
            dump(PROC[i].trace, ("dtrmv_ltu.tr." + std::to_string(i)).c_str());
        }
    }

    cout << "dtrmv_ltu [" << setw(2) << count << "] ";
    cout << "(n = " << setw(3) << n;
    cout << ", lda = " << setw(3) << lda;
    cout << ", incx = " << setw(2) << incx;
    cout << ", # of instr = " << setw(9);
    for (u32 p = 0; p < params::Proc::N; p++) {
        cout << setw(9) << PROC[p].instr_count;
    }
    cout << ", # of cycles = ";
    for (u32 p = 0; p < params::Proc::N; p++) {
        cout << setw(9) << PROC[p].op_maxcycle;
        if (PROC[p].op_maxcycle > 0) {
            num_procs++;
            max_cycles = max(max_cycles, PROC[p].op_maxcycle);
            total_cycles += PROC[p].op_maxcycle;
        }
    }
    cout << ", max cycles = " << setw(9) << max_cycles  << ", total cycles = " << setw(9) << total_cycles << ", avg cycles = " <<  setw(9) << total_cycles / (1.0 * num_procs);
    cout << ") : ";
    if (pass)
        cout << "PASS" << std::endl;
    else
        cout << "FAIL" << std::endl;
    
    CDC8600::memfree(x, nx);
    CDC8600::memfree(A, n*lda);
}

int main(int argc, char **argv)
{
    if (argc == 1) {
        for (u32 i = 0; i < N; i++)
        {
            i32 n = rand() % 256;
            i32 lda = n + rand() % 256;
            lda = max(lda, 1);
            i32 incx = (rand() % 16) - 8; // incx = 0?
            incx = (incx == 0) ? 1:incx;
            test_dtrmv_ltu(i, false, n, lda, incx);
        }
    } else if (argc == 4) {
        i32 n = atoi(argv[1]);
        i32 lda = atoi(argv[2]);
        i32 incx = atoi(argv[3]);
        if (incx < 1) {
            cerr << "icx must be larger than 0" << endl;
            exit(1);
        }
        if (lda < n || lda < 1) {
            cerr << "lda must be equal to or larger than 1 or n" << endl;
            exit(1);
        }
        test_dtrmv_ltu(0, true, n, lda, incx);

    } else {
        cerr << "Usage : " << argv[0] << "[n lda incx]" << endl;
	    exit(1);
    }
    return 0;
}