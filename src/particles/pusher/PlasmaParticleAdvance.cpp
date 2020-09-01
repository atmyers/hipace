#include "PlasmaParticleAdvance.H"

#include "particles/PlasmaParticleContainer.H"
#include "FieldGather.H"
#include "PushPlasmaParticles.H"
#include "UpdateForceTerms.H"
#include "fields/Fields.H"
#include "Constants.H"
#include "Hipace.H"
#include "GetAndSetPosition.H"

void
AdvancePlasmaParticles (PlasmaParticleContainer& plasma, Fields & fields,
                        amrex::Geometry const& gm, const ToSlice current_depo_type,
                        const bool do_push, const bool do_update, const bool do_shift,
                        int const lev)
{
    BL_PROFILE("UpdateForcePushParticles_PlasmaParticleContainer()");
    using namespace amrex::literals;

    // Extract properties associated with physical size of the box
    amrex::Real const * AMREX_RESTRICT dx = gm.CellSize();
    const PhysConst phys_const = get_phys_const();

    // Loop over particle boxes
    for (PlasmaParticleIterator pti(plasma, lev); pti.isValid(); ++pti)
    {
        // Extract properties associated with the extent of the current box
        // Grow to capture the extent of the particle shape
        amrex::Box tilebox = pti.tilebox().grow(
            {Hipace::m_depos_order_xy, Hipace::m_depos_order_xy, 0});

        amrex::RealBox const grid_box{tilebox, gm.CellSize(), gm.ProbLo()};
        amrex::Real const * AMREX_RESTRICT xyzmin = grid_box.lo();
        amrex::Dim3 const lo = amrex::lbound(tilebox);

        // Extract the fields
        const amrex::MultiFab& S = fields.getSlices(lev, 1);
        const amrex::MultiFab exmby(S, amrex::make_alias, FieldComps::ExmBy, 1);
        const amrex::MultiFab eypbx(S, amrex::make_alias, FieldComps::EypBx, 1);
        const amrex::MultiFab ez(S, amrex::make_alias, FieldComps::Ez, 1);
        const amrex::MultiFab bx(S, amrex::make_alias, FieldComps::Bx, 1);
        const amrex::MultiFab by(S, amrex::make_alias, FieldComps::By, 1);
        const amrex::MultiFab bz(S, amrex::make_alias, FieldComps::Bz, 1);
        // Extract FabArray for this box
        const amrex::FArrayBox& exmby_fab = exmby[pti];
        const amrex::FArrayBox& eypbx_fab = eypbx[pti];
        const amrex::FArrayBox& ez_fab = ez[pti];
        const amrex::FArrayBox& bx_fab = bx[pti];
        const amrex::FArrayBox& by_fab = by[pti];
        const amrex::FArrayBox& bz_fab = bz[pti];
        // Extract field array from FabArray
        amrex::Array4<const amrex::Real> const& exmby_arr = exmby_fab.array();
        amrex::Array4<const amrex::Real> const& eypbx_arr = eypbx_fab.array();
        amrex::Array4<const amrex::Real> const& ez_arr = ez_fab.array();
        amrex::Array4<const amrex::Real> const& bx_arr = bx_fab.array();
        amrex::Array4<const amrex::Real> const& by_arr = by_fab.array();
        amrex::Array4<const amrex::Real> const& bz_arr = bz_fab.array();

        const amrex::GpuArray<amrex::Real, 3> dx_arr = {dx[0], dx[1], dx[2]};
        const amrex::GpuArray<amrex::Real, 3> xyzmin_arr = {xyzmin[0], xyzmin[1], xyzmin[2]};

        auto& soa = pti.GetStructOfArrays(); // For momenta and weights

        // loading the data
        amrex::Real * const uxp = soa.GetRealData(PlasmaIdx::ux).data();
        amrex::Real * const uyp = soa.GetRealData(PlasmaIdx::uy).data();
        amrex::Real * const psip = soa.GetRealData(PlasmaIdx::psi).data();

        amrex::Real * const x_temp = soa.GetRealData(PlasmaIdx::x_temp).data();
        amrex::Real * const y_temp = soa.GetRealData(PlasmaIdx::y_temp).data();
        amrex::Real * const ux_temp = soa.GetRealData(PlasmaIdx::ux_temp).data();
        amrex::Real * const uy_temp = soa.GetRealData(PlasmaIdx::uy_temp).data();
        amrex::Real * const psi_temp = soa.GetRealData(PlasmaIdx::psi_temp).data();

        amrex::Real * const Fx1 = soa.GetRealData(PlasmaIdx::Fx1).data();
        amrex::Real * const Fy1 = soa.GetRealData(PlasmaIdx::Fy1).data();
        amrex::Real * const Fux1 = soa.GetRealData(PlasmaIdx::Fux1).data();
        amrex::Real * const Fuy1 = soa.GetRealData(PlasmaIdx::Fuy1).data();
        amrex::Real * const Fpsi1 = soa.GetRealData(PlasmaIdx::Fpsi1).data();
        amrex::Real * const Fx2 = soa.GetRealData(PlasmaIdx::Fx2).data();
        amrex::Real * const Fy2 = soa.GetRealData(PlasmaIdx::Fy2).data();
        amrex::Real * const Fux2 = soa.GetRealData(PlasmaIdx::Fux2).data();
        amrex::Real * const Fuy2 = soa.GetRealData(PlasmaIdx::Fuy2).data();
        amrex::Real * const Fpsi2 = soa.GetRealData(PlasmaIdx::Fpsi2).data();
        amrex::Real * const Fx3 = soa.GetRealData(PlasmaIdx::Fx3).data();
        amrex::Real * const Fy3 = soa.GetRealData(PlasmaIdx::Fy3).data();
        amrex::Real * const Fux3 = soa.GetRealData(PlasmaIdx::Fux3).data();
        amrex::Real * const Fuy3 = soa.GetRealData(PlasmaIdx::Fuy3).data();
        amrex::Real * const Fpsi3 = soa.GetRealData(PlasmaIdx::Fpsi3).data();
        amrex::Real * const Fx4 = soa.GetRealData(PlasmaIdx::Fx4).data();
        amrex::Real * const Fy4 = soa.GetRealData(PlasmaIdx::Fy4).data();
        amrex::Real * const Fux4 = soa.GetRealData(PlasmaIdx::Fux4).data();
        amrex::Real * const Fuy4 = soa.GetRealData(PlasmaIdx::Fuy4).data();
        amrex::Real * const Fpsi4 = soa.GetRealData(PlasmaIdx::Fpsi4).data();
        amrex::Real * const Fx5 = soa.GetRealData(PlasmaIdx::Fx5).data();
        amrex::Real * const Fy5 = soa.GetRealData(PlasmaIdx::Fy5).data();
        amrex::Real * const Fux5 = soa.GetRealData(PlasmaIdx::Fux5).data();
        amrex::Real * const Fuy5 = soa.GetRealData(PlasmaIdx::Fuy5).data();
        amrex::Real * const Fpsi5 = soa.GetRealData(PlasmaIdx::Fpsi5).data();

        const int depos_order_xy = Hipace::m_depos_order_xy;
        const amrex::Real clightsq = 1.0_rt/(phys_const.c*phys_const.c);

        const auto getPosition = GetParticlePosition(pti);
        const auto SetPosition = SetParticlePosition(pti);
        const amrex::Real zmin = xyzmin[2];
        const amrex::Real dz = dx[2];

        amrex::ParallelFor(pti.numParticles(),
            [=] AMREX_GPU_DEVICE (long ip) {

                amrex::ParticleReal xp, yp, zp;
                getPosition(ip, xp, yp, zp);
                // define field at particle position reals
                amrex::ParticleReal ExmByp = 0._rt, EypBxp = 0._rt, Ezp = 0._rt;
                amrex::ParticleReal Bxp = 0._rt, Byp = 0._rt, Bzp = 0._rt;

                if (do_shift)
                {
                    ShiftForceTerms(Fx1[ip], Fy1[ip], Fux1[ip], Fuy1[ip], Fpsi1[ip],
                                    Fx2[ip], Fy2[ip], Fux2[ip], Fuy2[ip], Fpsi2[ip],
                                    Fx3[ip], Fy3[ip], Fux3[ip], Fuy3[ip], Fpsi3[ip],
                                    Fx4[ip], Fy4[ip], Fux4[ip], Fuy4[ip], Fpsi4[ip],
                                    Fx5[ip], Fy5[ip], Fux5[ip], Fuy5[ip], Fpsi5[ip] );
                }

                if (do_update)
                {
                    // field gather for a single particle
                    doGatherShapeN(xp, yp, zmin,
                                   ExmByp, EypBxp, Ezp, Bxp, Byp, Bzp,
                                   exmby_arr, eypbx_arr, ez_arr, bx_arr, by_arr, bz_arr,
                                   dx_arr, xyzmin_arr, lo, depos_order_xy, 0);
                    // update force terms for a single particle
                    UpdateForceTerms(uxp[ip], uyp[ip], psip[ip], ExmByp, EypBxp, Ezp,
                                     Bxp, Byp, Bzp, Fx1[ip], Fy1[ip], Fux1[ip], Fuy1[ip],
                                     Fpsi1[ip], clightsq);
                }

                if (do_push)
                {
                    // push a single particle
                    PlasmaParticlePush(xp, yp, zp, uxp[ip], uyp[ip], psip[ip], x_temp[ip],
                                       y_temp[ip], ux_temp[ip], uy_temp[ip], psi_temp[ip],
                                       Fx1[ip], Fy1[ip], Fux1[ip], Fuy1[ip], Fpsi1[ip],
                                       Fx2[ip], Fy2[ip], Fux2[ip], Fuy2[ip], Fpsi2[ip],
                                       Fx3[ip], Fy3[ip], Fux3[ip], Fuy3[ip], Fpsi3[ip],
                                       Fx4[ip], Fy4[ip], Fux4[ip], Fuy4[ip], Fpsi4[ip],
                                       Fx5[ip], Fy5[ip], Fux5[ip], Fuy5[ip], Fpsi5[ip],
                                       dz, current_depo_type, ip, SetPosition );
                }
          }
          );
      }
}