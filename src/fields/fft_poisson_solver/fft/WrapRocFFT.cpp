/* Copyright 2019-2020
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause-LBNL
 */

#include "AnyFFT.H"
#include "RocFFTUtils.H"

namespace AnyFFT
{
    FFTplan CreatePlan (const amrex::IntVect& real_size, amrex::Real * const real_array,
                        Complex * const complex_array, const direction dir)
    {
        const int dim = 2;
        FFTplan fft_plan;

        const std::size_t lengths[] = {AMREX_D_DECL(std::size_t(real_size[0]),
                                                    std::size_t(real_size[1]),
                                                    std::size_t(real_size[2]))};
        // Initialize fft_plan.m_plan with the vendor fft plan.
        rocfft_status result = rocfft_plan_create(&(fft_plan.m_plan),
                                                  rocfft_placement_notinplace,
                                                  (dir == direction::R2C)
                                                      ? rocfft_transform_type_real_forward
                                                      : rocfft_transform_type_real_inverse,
#ifdef AMREX_USE_FLOAT
                                                  rocfft_precision_single,
#else
                                                  rocfft_precision_double,
#endif
                                                  dim, lengths,
                                                  1, // number of transforms,
                                                  nullptr);
        RocFFTUtils::assert_rocfft_status("rocfft_plan_create", result);

        // Store meta-data in fft_plan
        fft_plan.m_real_array = real_array;
        fft_plan.m_complex_array = complex_array;
        fft_plan.m_dir = dir;
        //fft_plan.m_dim = dim;

        return fft_plan;
    }

    void DestroyPlan (FFTplan& fft_plan)
    {
        rocfft_plan_destroy( fft_plan.m_plan );
    }

    void Execute (FFTplan& fft_plan)
    {
        rocfft_execution_info execinfo = NULL;
        rocfft_status result = rocfft_execution_info_create(&execinfo);
        RocFFTUtils::assert_rocfft_status("rocfft_execution_info_create", result);

        std::size_t buffersize = 0;
        result = rocfft_plan_get_work_buffer_size(fft_plan.m_plan, &buffersize);
        RocFFTUtils::assert_rocfft_status("rocfft_plan_get_work_buffer_size", result);

        void* buffer = amrex::The_Arena()->alloc(buffersize);
        result = rocfft_execution_info_set_work_buffer(execinfo, buffer, buffersize);
        RocFFTUtils::assert_rocfft_status("rocfft_execution_info_set_work_buffer", result);

        result = rocfft_execution_info_set_stream(execinfo, amrex::Gpu::gpuStream());
        RocFFTUtils::assert_rocfft_status("rocfft_execution_info_set_stream", result);

        if (fft_plan.m_dir == direction::R2C) {
            result = rocfft_execute(fft_plan.m_plan,
                                    (void**)&(fft_plan.m_real_array), // in
                                    (void**)&(fft_plan.m_complex_array), // out
                                    execinfo);
        } else if (fft_plan.m_dir == direction::C2R) {
            result = rocfft_execute(fft_plan.m_plan,
                                    (void**)&(fft_plan.m_complex_array), // in
                                    (void**)&(fft_plan.m_real_array), // out
                                    execinfo);
        } else {
            amrex::Abort("direction must be AnyFFT::direction::R2C or AnyFFT::direction::C2R");
        }

        RocFFTUtils::assert_rocfft_status("rocfft_execute", result);

        amrex::Gpu::streamSynchronize();

        amrex::The_Arena()->free(buffer);

        result = rocfft_execution_info_destroy(execinfo);
        RocFFTUtils::assert_rocfft_status("rocfft_execution_info_destroy", result);
    }
}
