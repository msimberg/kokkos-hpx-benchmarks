/*
//@HEADER
// ************************************************************************
//
//                        Kokkos v. 2.0
//              Copyright (2014) Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact  H. Carter Edwards (hcedwar@sandia.gov)
//
// ************************************************************************
//@HEADER
*/

#include <Kokkos_Core.hpp>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/time.h>
#include <unistd.h>

#if defined(KOKKOS_ENABLE_DEFAULT_DEVICE_TYPE_OPENMP)
#define KOKKOS_DEVICE "OpenMP"
#elif defined(KOKKOS_ENABLE_DEFAULT_DEVICE_TYPE_OPENMPTARGET)
#define KOKKOS_DEVICE "OpenMPTarget"
#elif defined(KOKKOS_ENABLE_DEFAULT_DEVICE_TYPE_THREADS)
#define KOKKOS_DEVICE "Threads"
#elif defined(KOKKOS_ENABLE_DEFAULT_DEVICE_TYPE_SERIAL)
#define KOKKOS_DEVICE "Serial"
#else
#define KOKKOS_DEVICE "Unknown"
#endif

int main(int argc, char *argv[]) {

  int num_vectors = 10000; // number of vectors
  int len = 100000;        // length of vectors
  int nrepeat = 10;        // number of repeats of the test
  bool header = false;

  // Read command line arguments
  for (int i = 0; i < argc; i++) {
    if ((strcmp(argv[i], "-v") == 0) ||
        (strcmp(argv[i], "-num_vectors") == 0)) {
      num_vectors = atoi(argv[++i]);
    } else if ((strcmp(argv[i], "-l") == 0) || (strcmp(argv[i], "-v") == 0)) {
      len = atof(argv[++i]);
    } else if (strcmp(argv[i], "-nrepeat") == 0) {
      nrepeat = atoi(argv[++i]);
    } else if (strcmp(argv[i], "-header") == 0) {
      header = true;
    } else if ((strcmp(argv[i], "-h") == 0) ||
               (strcmp(argv[i], "-help") == 0)) {
      printf("ArrayOfDotProducts Options:\n");
      printf("  -num_vectors (-v)  <int>: number of vectors (default: 1000)\n");
      printf("  -length (-l) <int>:       vector length (default: 10000)\n");
      printf("  -nrepeat <int>:           number of repitions (default: 10)\n");
      printf("  -help (-h):               print this message\n");
    }
  }

  char hostname[HOST_NAME_MAX];
  gethostname(hostname, HOST_NAME_MAX);
  std::string benchmark = "array_of_dot_products";
  std::string runtime = "Kokkos-Minimal-" + std::string(KOKKOS_DEVICE);

  Kokkos::initialize(argc, argv);
  {
    // allocate space for vectors to do num_vectors dot products of length len
    double *a = new double[num_vectors * len];
    double *b = new double[num_vectors * len];
    double *c = new double[num_vectors];

    // Initialize vectors
    Kokkos::parallel_for(num_vectors, KOKKOS_LAMBDA(const int &i) {
      for (int j = 0; j < len; j++) {
        a[i * len + j] = i + 1;
        b[i * len + j] = j + 1;
      }
      c[i] = 0.0;
    });

    // Time dot products
    struct timeval begin, end;

    gettimeofday(&begin, NULL);

    for (int repeat = 0; repeat < nrepeat; repeat++) {
      Kokkos::parallel_for(num_vectors, KOKKOS_LAMBDA(const int &i) {
        double ctmp = 0.0;
        for (int j = 0; j < len; j++) {
          ctmp += a[i * len + j] * b[i * len + j];
        }
        c[i] = ctmp;
      });
    }

    gettimeofday(&end, NULL);

    // Calculate time
    double time = 1.0 * (end.tv_sec - begin.tv_sec) +
                  1.0e-6 * (end.tv_usec - begin.tv_usec);

    // Error check
    int error = 0;
    for (int i = 0; i < num_vectors; i++) {
      double diff = ((c[i] - 1.0 * (i + 1) * len * (len + 1) / 2)) /
                    ((i + 1) * len * (len + 1) / 2);
      if (diff * diff > 1e-20) {
        error = 1;
      }
    }

    if (error != 0) {
      std::cerr << "Error!" << std::endl;
    }

    if (header) {
      std::cout << "hostname, timestamp, num_threads, benchmark, runtime "
                   "input_size_1, input_size_2, num_repeats, time, result, "
                   "specific_metric, metric_name"
                << std::endl;
    }
    std::cout << hostname << ", " << std::time(nullptr) << ", "
              << Kokkos::DefaultExecutionSpace::concurrency() << ", "
              << benchmark << ", " << runtime << ", " << len << ", "
              << num_vectors << ", " << nrepeat << ", " << time << ", " << error
              << ", " << 1.0e-9 * num_vectors * len * 2 * 8 * nrepeat / time
              << ", bandwidth_GB_per_s" << std::endl;
  }

  Kokkos::finalize();
}
