/* Copyright 2020 Axel Huebl, Maxence Thevenet
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause-LBNL
 */

#ifndef HIPACE_PROFILERWRAPPER_H_
#define HIPACE_PROFILERWRAPPER_H_

#include "Hipace.H"

#include <AMReX_BLProfiler.H>
#include <AMReX_GpuDevice.H>

template<int detail_level>
AMREX_FORCE_INLINE
void doDeviceSynchronize ()
{
    if ( Hipace::m_do_device_synchronize >= detail_level )
        amrex::Gpu::synchronize();
}

// Note that objects are destructed in the reverse order of declaration
template<int detail_level>
struct synchronizeOnDestruct {
    AMREX_FORCE_INLINE
    ~synchronizeOnDestruct () {
        doDeviceSynchronize<detail_level>();
    }
};

#define HIPACE_PROFILE(fname) doDeviceSynchronize<1>(); BL_PROFILE(fname); synchronizeOnDestruct<1> SYNC_SCOPE{}
#define HIPACE_PROFILE_VAR(fname, vname) doDeviceSynchronize<1>(); BL_PROFILE_VAR(fname, vname); synchronizeOnDestruct<1> SYNC_V_##vname{}
#define HIPACE_PROFILE_VAR_NS(fname, vname) BL_PROFILE_VAR_NS(fname, vname); synchronizeOnDestruct<1> SYNC_V_##vname{}
#define HIPACE_PROFILE_VAR_START(vname) doDeviceSynchronize<1>(); BL_PROFILE_VAR_START(vname)
#define HIPACE_PROFILE_VAR_STOP(vname) doDeviceSynchronize<1>(); BL_PROFILE_VAR_STOP(vname)
#define HIPACE_PROFILE_REGION(rname) doDeviceSynchronize<1>(); BL_PROFILE_REGION(rname); synchronizeOnDestruct<1> SYNC_R_##rname{}

#define HIPACE_DETAIL_PROFILE(fname) doDeviceSynchronize<2>(); BL_PROFILE(fname); synchronizeOnDestruct<2> SYNC_SCOPE{}
#define HIPACE_DETAIL_PROFILE_VAR(fname, vname) doDeviceSynchronize<2>(); BL_PROFILE_VAR(fname, vname); synchronizeOnDestruct<2> SYNC_V_##vname{}
#define HIPACE_DETAIL_PROFILE_VAR_NS(fname, vname) BL_PROFILE_VAR_NS(fname, vname); synchronizeOnDestruct<2> SYNC_V_##vname{}
#define HIPACE_DETAIL_PROFILE_VAR_START(vname) doDeviceSynchronize<2>(); BL_PROFILE_VAR_START(vname)
#define HIPACE_DETAIL_PROFILE_VAR_STOP(vname) doDeviceSynchronize<2>(); BL_PROFILE_VAR_STOP(vname)
#define HIPACE_DETAIL_PROFILE_REGION(rname) doDeviceSynchronize<2>(); BL_PROFILE_REGION(rname); synchronizeOnDestruct<2> SYNC_R_##rname{}

#endif // HIPACE_PROFILERWRAPPER_H_
