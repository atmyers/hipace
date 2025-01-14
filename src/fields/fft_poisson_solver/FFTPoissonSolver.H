#ifndef FFT_POISSON_SOLVER_H_
#define FFT_POISSON_SOLVER_H_

#include "fields/fft_poisson_solver/fft/AnyFFT.H"

#include <AMReX_MultiFab.H>
#include <AMReX_GpuComplex.H>

/**
 * \brief This class handles functions and data to perform transverse Fourier-based Poisson solves.
 *
 * For a given source S, it solves equation Laplacian(F) = S and returns F.
 * Once an instance is created, a typical use consists in:
 * 1. Compute S directly in FFTPoissonSolver::m_stagingArea
 * 2. Call FFTPoissonSolver::SolvePoissonEquation(mf), which will solve Poisson equation with RHS
 *    in the staging area and return the LHS in mf.
 */
class FFTPoissonSolver
{
public:

    /** Default constructor */
    FFTPoissonSolver () = default;

    /** Abstract class needs a virtual destructor */
    virtual ~FFTPoissonSolver () = 0;

    /**
     * \brief Define real space and spectral space boxes and multifabs, multiplier
     * coefficients inv_k2 to solve Poisson equation and FFT plans.
     * Currently only works with a single box, i.e., serial FFT.
     *
     * \param[in] realspace_ba BoxArray on which the FFT is executed.
     * \param[in] dm DistributionMapping for the BoxArray.
     * \param[in] gm Geometry, contains the box dimensions.
     */
    virtual void define ( amrex::BoxArray const& realspace_ba,
                          amrex::DistributionMapping const& dm,
                          amrex::Geometry const& gm) = 0;

    /**
     * Solve Poisson equation. The source term must be stored in the staging area m_stagingArea prior to this call.
     *
     * \param[in] lhs_mf Destination array, where the result is stored.
     */
    virtual void SolvePoissonEquation (amrex::MultiFab& lhs_mf) = 0;

    /** Get reference to the taging area */
    amrex::MultiFab& StagingArea ();
protected:
    /** BoxArray for the spectral fields */
    amrex::BoxArray m_spectralspace_ba;
    /** Staging area, contains (real) field in real space.
     * This is where the source term is stored before calling the Poisson solver */
    amrex::MultiFab m_stagingArea;
};

#endif
