//LIC// ====================================================================
//LIC// This file forms part of oomph-lib, the object-oriented, 
//LIC// multi-physics finite-element library, available 
//LIC// at http://www.oomph-lib.org.
//LIC// 
//LIC//    Version 1.0; svn revision $LastChangedRevision: 1097 $
//LIC//
//LIC// $LastChangedDate: 2015-12-17 11:53:17 +0000 (Thu, 17 Dec 2015) $
//LIC// 
//LIC// Copyright (C) 2006-2016 Matthias Heil and Andrew Hazel
//LIC// 
//LIC// This library is free software; you can redistribute it and/or
//LIC// modify it under the terms of the GNU Lesser General Public
//LIC// License as published by the Free Software Foundation; either
//LIC// version 2.1 of the License, or (at your option) any later version.
//LIC// 
//LIC// This library is distributed in the hope that it will be useful,
//LIC// but WITHOUT ANY WARRANTY; without even the implied warranty of
//LIC// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//LIC// Lesser General Public License for more details.
//LIC// 
//LIC// You should have received a copy of the GNU Lesser General Public
//LIC// License along with this library; if not, write to the Free Software
//LIC// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
//LIC// 02110-1301  USA.
//LIC// 
//LIC// The authors may be contacted at oomph-lib@maths.man.ac.uk.
//LIC// 
//LIC//====================================================================
// Header file for the Biharmonic Bell elements
#ifndef OOMPH_MYBIHARMONIC_ELEMENTS_HEADER
#define OOMPH_MYBIHARMONIC_ELEMENTS_HEADER


#include<sstream>

//OOMPH-LIB headers
#include "../generic/nodes.h"
#include "../generic/oomph_utilities.h"
#include "../generic/Telements.h"


namespace oomph
{
//=============================================================
/// A class for all subparametric elements that solve the 2D-
/// Biharmonic equations.
/// \f[
/// \frac{\partial^4 u}{\partial x_i^4} = f(x_j)
/// \f]
/// This contains the generic maths. Shape functions, geometric
/// mapping etc. must get implemented in derived class.
//=============================================================
//HERE should we get rid of DIM as template? Seems misleading
template <unsigned DIM, unsigned NNODE_1D>
class FoepplVonKarmanEquations : public virtual TElement<DIM,NNODE_1D>
{

public:

 /// \short Function pointer to pressure function fct(x,f(x)) --
 /// x is a Vector!
 typedef void (*PressureFctPt)(const Vector<double>& x, double& f);


 /// \short Function pointer to in plane forcing function  fct(x,g(x)) --
 /// x is a Vector!
 typedef void (*InPlaneForcingFctPt)(const Vector<double>& x,
                                            Vector<double>& forcing);

 /// \short Function pointer to the Error Metric we are using
 ///  e.g could be that we are just interested in error on w etc.
 typedef void (*ErrorMetricFctPt)(const Vector<double>& x, const 
  Vector<double>& u, const Vector<double>& u_exact, double& error, double& norm);

 /// \short Function pointer to the Error Metric we are using if we want multiple
 ///  errors.  e.g could be we want errors seperately on each displacment
 typedef void (*MultipleErrorMetricFctPt)(const Vector<double>& x, const 
  Vector<double>& u, const Vector<double>& u_exact, Vector<double>& error, 
  Vector<double>& norm);

 /// \short (pure virtual) interface to fixing the out of plane displacement
 virtual void fix_out_of_plane_displacement_dof(const unsigned& dof_number, 
const unsigned& boundary_number, const PressureFctPt& w)=0;
 // NB do we need this?

 /// \short (pure virtual) interface to fixing the in plane displacement
 virtual void fix_in_plane_displacement_dof(const unsigned& dof_number,
const unsigned& boundary_number, const PressureFctPt& u)=0;
 // NB do we need this?
 
 /// \short (Read only) Access to number of internal dofs
 unsigned number_of_internal_dofs() const {return this->Number_of_internal_dofs;}

 /// \short (pure virtual) function that precomputes association matrix between
 /// the basis functions of the basic element and the physical element.
 virtual void precompute_association_matrix(DenseMatrix<double>& m)=0;
 
 /// short (pure virtual) function that returns the number of basis functions in 
 /// the physical element
 virtual double n_basis_functions()=0;

 /// short (pure virtual) function that returns the number of basis functions in
 /// the basic element
 virtual double n_basic_basis_functions()=0;

 
 public:

 /// Get pointer to association matrix 
 DenseMatrix<double> *get_association_matrix_pt()const {return Association_matrix_pt;};

 public: 

 /// Eta
 const double &eta() const {return *Eta_pt;}

 /// Pointer to eta
 const double* &eta_pt() {return Eta_pt;}

 /// Pure virtual function to pin all deflection dofs
 virtual void pin_all_deflection_dofs() const=0;

 /// Constructor (must initialise the Pressure_fct_pt to null)
 FoepplVonKarmanEquations() : Pressure_fct_pt(0),
   In_plane_forcing_fct_pt(0),Number_of_internal_dofs(0),
   Number_of_internal_dof_types(0), Error_metric_fct_pt(0),
    Multiple_error_metric_fct_pt(0), Association_matrix_pt(0) 
  {
   Eta_pt = &Default_Eta_Value;
   Nu_pt = &Default_Nu_Value;
  }

 /// Broken copy constructor
 FoepplVonKarmanEquations(const FoepplVonKarmanEquations& dummy)
  {
   BrokenCopy::broken_copy("FoepplVonKarmanEquations");
  }

 /// Broken assignment operator
 void operator=(const FoepplVonKarmanEquations&)
  {
   BrokenCopy::broken_assign("FoepplVonKarmanEquations");
  }

 /// \short Return the index at which the first u unknown value
 /// is stored.
 /// In derived multi-physics elements, this function should be overloaded
 /// to reflect the chosen storage scheme. Note that these equations require
 /// that the unknown is always stored at the same index at each node.
 /// Note these are stored before w_dofs otherwise at vertex nodes the 
 /// stored value would be at a different index
 virtual inline unsigned u_nodal_index_foeppl_von_karman() const {return 0;}

 /// \short Return the index at which the first w unknown value
 /// is stored.
 /// In derived multi-physics elements, this function should be overloaded
 /// to reflect the chosen storage scheme. Note that these equations require
 /// that the unknown is always stored at the same index at each node.
 /// Note that at midside nodes there are no w dofs 
 virtual inline unsigned w_nodal_index_foeppl_von_karman() const {return u_nodal_index_foeppl_von_karman()+2;}

 /// Output with default number of plot points
 void output(std::ostream &outfile)
  {
   const unsigned n_plot=5;
   FoepplVonKarmanEquations::output(outfile,n_plot);
  }

 /// \short Output FE representation of soln: x,y,u or x,y,z,u at
 /// n_plot^DIM plot points
 void output(std::ostream &outfile, const unsigned &n_plot);

 /// C_style output with default number of plot points
 void output(FILE* file_pt)
  {
   const unsigned n_plot=5;
   FoepplVonKarmanEquations::output(file_pt,n_plot);
  }

 /// \short C-style output FE representation of soln: x,y,u or x,y,z,u at
 /// n_plot^DIM plot points
 void output(FILE* file_pt, const unsigned &n_plot);

 /// Output exact soln: x,y,u_exact or x,y,z,u_exact at n_plot^DIM plot points
 void output_fct(std::ostream &outfile, const unsigned &n_plot,
                 FiniteElement::SteadyExactSolutionFctPt exact_soln_pt);

 /// \short Output exact soln: x,y,u_exact or x,y,z,u_exact at
 /// n_plot^DIM plot points (dummy time-dependent version to
 /// keep intel compiler happy)
 virtual void output_fct(std::ostream &outfile, const unsigned &n_plot,
                         const double& time,
                         FiniteElement::UnsteadyExactSolutionFctPt
                         exact_soln_pt)
  {
   throw OomphLibError(
    "There is no time-dependent output_fct() for these elements ",
    OOMPH_CURRENT_FUNCTION, OOMPH_EXCEPTION_LOCATION);
  }


 /// Get error against and norm of exact solution
 void compute_error(std::ostream &outfile,
                    FiniteElement::SteadyExactSolutionFctPt exact_soln_pt,
                    double& error, double& norm);

 /// Get error against and norm of exact solution
 void compute_error_in_deflection(std::ostream &outfile,
                    FiniteElement::SteadyExactSolutionFctPt exact_soln_pt,
                    double& error, double& norm);

 /// Dummy, time dependent error checker
 void compute_error(std::ostream &outfile,
                    FiniteElement::UnsteadyExactSolutionFctPt exact_soln_pt,
                    const double& time, double& error, double& norm)
  {
   throw OomphLibError(
    "There is no time-dependent compute_error() for these elements",
    OOMPH_CURRENT_FUNCTION, OOMPH_EXCEPTION_LOCATION);
  }

 /// Fill in the stress tensor
 void get_sigma(DenseMatrix<double>& sigma, const DenseMatrix<double>& grad_u,
  const DenseMatrix<double>& grad_w )const
  {
   // Poisson ratio
   double nu(get_nu());
   // Truncated Green Lagrange strain tensor
   DenseMatrix<double> epsilon(DIM,DIM,0.0);
   for(unsigned alpha=0;alpha<DIM;++alpha)
    {
     for(unsigned beta=0;beta<DIM;++beta)
      {
       // Truncated Green Lagrange strain tensor
       epsilon(alpha,beta) += 0.5* grad_u(alpha,beta) + 0.5*grad_u(beta,alpha)
                           +0.5*grad_w(0,alpha)*grad_w(0,beta);
      }
    }
   
   // Now construct the Stress
   for(unsigned alpha=0;alpha<DIM;++alpha)
    {
     for(unsigned beta=0;beta<DIM;++beta)
      {
       // The Laplacian term: Trace[ \epsilon ] I
       // \nu * \epsilon_{\alpha \beta} delta_{\gamma \gamma}
       sigma(alpha,alpha) += nu*epsilon(beta,beta)/(1-nu*nu);
      
       // The scalar transform term: \epsilon
       // (1-\nu) * \epsilon_{\alpha \beta}
       sigma(alpha,beta) += (1-nu)* epsilon(alpha,beta)/(1-nu*nu);
      }
    }
 
  }
 /// Access function: Pointer to pressure function
 PressureFctPt& pressure_fct_pt() {return Pressure_fct_pt;}

 /// Access function: Pointer to pressure function. Const version
 PressureFctPt pressure_fct_pt() const {return Pressure_fct_pt;}

 /// Access function: Pointer to error metric function
 ErrorMetricFctPt& error_metric_fct_pt() {return Error_metric_fct_pt;}

 /// Access function: Pointer to multiple error metric function
 MultipleErrorMetricFctPt& multiple_error_metric_fct_pt() 
  {return Multiple_error_metric_fct_pt;}

 /// Access function: Pointer to error metric function function
 ErrorMetricFctPt error_metric_fct_pt() const {return Error_metric_fct_pt;}

 /// Access function: Pointer to multiple error metric function
 MultipleErrorMetricFctPt multiple_error_metric_fct_pt() const 
  {return Multiple_error_metric_fct_pt;}

 /// Access function: Pointer to in plane forcing function
 InPlaneForcingFctPt& in_plane_forcing_fct_pt()
  {return In_plane_forcing_fct_pt;}

 /// Access function: Pointer to in plane forcing function. Const version
 InPlaneForcingFctPt in_plane_forcing_fct_pt() const
  {return In_plane_forcing_fct_pt;}

 ///Access function to the Poisson ratio.
 const double*& nu_pt() {return Nu_pt;}

 ///Access function to the Poisson ratio (const version)
 const double& get_nu() const {return *Nu_pt;}

 /// Get the kth dof type at internal point l
 virtual double get_w_bubble_dof(const unsigned& l, const unsigned& k) const =0;

 /// Get the kth equation at internal point l
 virtual int local_w_bubble_equation(const unsigned& l, const unsigned& k)
   const =0;

 /// Get pressure term at (Eulerian) position x. This function is
 /// virtual to allow overloading in multi-physics problems where
 /// the strength of the pressure function might be determined by
 /// another system of equations.
 inline virtual void get_pressure_foeppl_von_karman(const unsigned& ipt,
                                        const Vector<double>& x,
                                        double& pressure) const
  {
   //If no pressure function has been set, return zero
   if(Pressure_fct_pt==0)
    {
     pressure = 0.0;
    }
   else
    {
     // Get pressure strength
     (*Pressure_fct_pt)(x,pressure);
    }
  }

 /// Get pressure term at (Eulerian) position x. This function is
 /// virtual to allow overloading in multi-physics problems where
 /// the strength of the pressure function might be determined by
 /// another system of equations.
 inline virtual void get_in_plane_forcing_foeppl_von_karman(const unsigned& ipt,
                                        const Vector<double>& x,
                                        Vector<double>& pressure) const
  {
   //In plane is same as DIM of problem (2)
   pressure.resize(DIM);
   //If no pressure function has been set, return zero
   if(In_plane_forcing_fct_pt==0)
    {
     pressure[0] = 0.0;
     pressure[1] = 0.0; 
    }
   else
    {
     // Get pressure strength
     (*In_plane_forcing_fct_pt)(x,pressure);
    }
  }

 /// Add the element's contribution to its residual vector (wrapper)
 void fill_in_contribution_to_residuals(Vector<double> &residuals)
  {
   // Precompute the association matrix
   DenseMatrix<double> conversion_matrix (n_basis_functions(),
    n_basic_basis_functions(),0.0);
   this->precompute_association_matrix(conversion_matrix);
   this->Association_matrix_pt=&conversion_matrix;

   //Call the generic residuals function with flag set to 0
   //using a dummy matrix argument
   fill_in_generic_residual_contribution_foeppl_von_karman(
    residuals,GeneralisedElement::Dummy_matrix,0);

   Association_matrix_pt=0;
  }


 /// Add the element's contribution to its residual vector and
 /// element Jacobian matrix (wrapper)
 void fill_in_contribution_to_jacobian(Vector<double> &residuals,
                                   DenseMatrix<double> &jacobian)
  {
   DenseMatrix<double> conversion_matrix (n_basis_functions(),
    n_basic_basis_functions(),0.0);
   this->precompute_association_matrix(conversion_matrix);
   this->Association_matrix_pt=&conversion_matrix;

   //Call the generic routine with the flag set to 1
   fill_in_generic_residual_contribution_foeppl_von_karman(residuals,jacobian,1);

   Association_matrix_pt=0;
  }

 /// Add the element's contribution to its residual vector and
 /// element Jacobian matrix (wrapper)
 void fill_in_contribution_to_jacobian_and_mass_matrix(Vector<double> &residuals,
                                   DenseMatrix<double> &jacobian,DenseMatrix<double> &mass_matrix)
  {
   //Call fill in Jacobian 
   fill_in_contribution_to_jacobian(residuals,jacobian);
   // There is no mass matrix: we will just want J w = 0

   // -- COPIED FROM DISPLACMENT FVK EQUATIONS --
   // Dummy diagonal (won't result in global unit matrix but
   // doesn't matter for zero eigenvalue/eigenvector
   unsigned ndof=mass_matrix.nrow();
   for (unsigned i=0;i<ndof;i++)
    {
     mass_matrix(i,i)+=1.0;
    }
  }


 /// \short Return FE representation of unknown values u(s)
 /// at local coordinate s
 virtual inline Vector<double> interpolated_u_foeppl_von_karman(const Vector<double> &s) const
  {
   //Find number of position dofs
   const unsigned n_position_type = this->nnodal_position_type();
   // Find the internal dofs
   const unsigned n_b_position_type = this->Number_of_internal_dof_types;
   //Find out how many nodes there are
   const unsigned n_w_node = 3;
  //Find out how many nodes there are
   const unsigned n_node = this->nnode();
   //Find out how many internal points there are
   const unsigned n_b_node = this->Number_of_internal_dofs;
   //Get the index at which the unknown is stored
   const unsigned u_nodal_index = u_nodal_index_foeppl_von_karman();
   //Get the index at which the unknown is stored
   const unsigned w_nodal_index = w_nodal_index_foeppl_von_karman();
   //Get the index at which the unknown is stored
   const unsigned w_bubble_index = 0; //w_index_foeppl_von_karman();
   // Number of unique second deriv. will be the DIMth triangle number
   const unsigned n_second_deriv = (DIM*(DIM+1))/2;

   //Local c1-shape funtion
   Shape psi(n_w_node,n_position_type),test(n_w_node,n_position_type),
    psi_b(n_b_node,n_b_position_type),test_b(n_b_node,n_b_position_type);
   
   DShape dpsi_dxi(n_w_node,n_position_type,DIM),dtest_dxi(n_w_node,n_position_type,DIM),
    dpsi_b_dxi(n_b_node,n_b_position_type,DIM),dtest_b_dxi(n_b_node,n_b_position_type,DIM),
    d2psi_dxi2(n_w_node,n_position_type,n_second_deriv), d2test_dxi2(n_w_node,n_position_type,n_second_deriv),
    d2psi_b_dxi2(n_b_node,n_b_position_type,n_second_deriv), d2test_b_dxi2(n_b_node,n_b_position_type,n_second_deriv);
   
   // In--plane dofs
   Shape psi_u(n_node);
   DShape dpsi_u(n_node,DIM);
   Shape test_u(n_node);
   DShape dtest_u(n_node,DIM);

   // Number of in-plane displacement fields is equal to dimension
   const unsigned n_u_fields = 2;// DIM;
   // Number of out-of-plane displacement fields is equal 1 (value of deflection_
   // plus first and second deriv
   const unsigned n_w_fields = 6;//1+DIM+n_second_deriv;
   //Initialise value of u
   Vector<double> interpolated_u(n_u_fields+n_w_fields,0.0);
   //Find values of c1-shape function
   d2shape_and_d2test_eulerian_foeppl_von_karman(s,psi,psi_b,dpsi_dxi,dpsi_b_dxi,
    d2psi_dxi2,d2psi_b_dxi2,test,test_b,dtest_dxi,dtest_b_dxi,d2test_dxi2,
    d2test_b_dxi2);

   // Get shape and test
   dshape_u_and_dtest_u_eulerian_foeppl_von_karman(s,psi_u,dpsi_u,test_u,dtest_u);
   //Interpolated unknown
   for(unsigned l=0;l<n_w_node;l++)
   {
    for(unsigned k=0;k<n_position_type;k++)
     {
     // u_3
     interpolated_u[0] += this->nodal_value(l,w_nodal_index+k)*psi(l,k);
     // d_u_3_dx_alpha
     for(unsigned alpha=0;alpha<DIM;++alpha)
      {interpolated_u[1+alpha] += this->nodal_value(l,w_nodal_index+k)*dpsi_dxi(l,k,alpha);}
     // d2_u_3_dx_alpha dx_beta
     for(unsigned alphabeta=0;alphabeta<n_second_deriv;++alphabeta)
      {
      interpolated_u[DIM+1+alphabeta] += this->nodal_value(l,w_nodal_index+k)
         *d2psi_dxi2(l,k,alphabeta);
      }
     }
   }

   // Bubble dofs
   for(unsigned l=0;l<Number_of_internal_dofs;l++)
   {
    for(unsigned k=0;k<Number_of_internal_dof_types;k++)
     {
      double u_value = get_w_bubble_dof(l,w_bubble_index+k);
      // u_3
      interpolated_u[0] += u_value * psi_b(l,w_bubble_index+k);
      // d_u_3_dx_alpha
      for(unsigned alpha=0;alpha<DIM;++alpha)
       { interpolated_u[1+alpha] += u_value*dpsi_b_dxi(l,w_bubble_index+k,alpha); }
      // d2_u_3_dx_alpha dx_beta
     for(unsigned alphabeta=0;alphabeta<n_second_deriv;++alphabeta)
      {
       interpolated_u[DIM+1+alphabeta] += u_value*d2psi_b_dxi2(l,w_bubble_index+k,alphabeta);
      }
     }
   }
   // Now for the displacement 
   for(unsigned l=0; l< n_node;++l)
    {
     // Now for the two in--plane displacements
     interpolated_u[6] += this->nodal_value(l,u_nodal_index+0)*psi_u(l);
     interpolated_u[7] += this->nodal_value(l,u_nodal_index+1)*psi_u(l);
    // // Also output the in--plane displacement derivatives
    // for(unsigned i=0; i<DIM; ++i)
    //  {
    //  interpolated_u[8 +i] += this->nodal_value(l,u_nodal_index+0)*dpsi_u(l,i);
    //  interpolated_u[10+i] += this->nodal_value(l,u_nodal_index+1)*dpsi_u(l,i);
    //  }
    }
   return(interpolated_u);
  }

 /// \short Self-test: Return 0 for OK
 unsigned self_test();

 /// \short get the coordinate
 virtual void get_coordinate_x(const Vector<double>& s, Vector<double>& x) const=0;
 
// /// Wrapper
// void interpolated_x(const Vector<double>& s, Vector<double>& x) const
//  {
//   this->get_coordinate_x(s,x);
//  }

protected:
 /// Pure virtual interface to the basis for the in--plane displacement 
 virtual void shape_u(const Vector<double> &s,  Shape &psi) const=0;

 /// \short Shape/test functions and derivs w.r.t. to global coords at
 /// local coord. s; return  Jacobian of mapping
 virtual double d2shape_and_d2test_eulerian_foeppl_von_karman(const Vector<double> &s,
  Shape &psi, Shape& psi_b, DShape &dpsi_dx, DShape &dpsi_b_dx,
  DShape &d2psi_dx2,DShape& d2psi_b_dx2,
  Shape &test, Shape& test_b, DShape &dtest_dx, DShape &dtest_b_dx,
  DShape &d2test_dx2,DShape& d2test_b_dx2) const=0;

 /// \short Shape/test functions and derivs w.r.t. to global coords at
 /// local coord. s; return  Jacobian of mapping
 virtual double dshape_and_dtest_eulerian_foeppl_von_karman(const Vector<double> &s,
  Shape &psi, Shape& psi_b, DShape &dpsi_dx, DShape &dpsi_b_dx,
  Shape &test, Shape& test_b, DShape &dtest_dx, DShape &dtest_b_dx) const=0;

 /// \short Shape/test functions at local coordinate s
 virtual void shape_and_test_foeppl_von_karman(const Vector<double> &s,
  Shape &psi, Shape& psi_b, Shape &test, Shape& test_b) const=0;
 
 /// \short in--plane Shape/test functions at local coordinate s
 virtual double dshape_u_and_dtest_u_eulerian_foeppl_von_karman(const Vector<double> &s,
  Shape &psi,DShape &dpsidx, Shape &test,DShape &dtestdx) const=0;

/// HERE BREAK THESE
// /// \short Shape/test functions and derivs w.r.t. to global coords at
// /// local coord. s; return  Jacobian of mapping
// virtual double d2shape_and_d2test_eulerian_at_knot_foeppl_von_karman(const
//  unsigned ipt, Shape &psi, Shape& psi_b, DShape &dpsi_dx, DShape &dpsi_b_dx,
//  DShape &d2psi_dx2,DShape& d2psi_b_dx2,
//  Shape &test, Shape& test_b, DShape &dtest_dx, DShape &dtest_b_dx,
//  DShape &d2test_dx2,DShape& d2test_b_dx2) const=0;
//
// /// \short Shape/test functions and derivs w.r.t. to global coords at
// /// local coord. s; return  Jacobian of mapping
// virtual double dshape_and_dtest_eulerian__at_knot_foeppl_von_karman(const
//  unsigned& ipt, Shape &psi, Shape& psi_b, DShape &dpsi_dx, DShape &dpsi_b_dx,
//  Shape &test, Shape& test_b, DShape &dtest_dx, DShape &dtest_b_dx) const=0;
//
// /// \short Shape/test functions at integral point ipt
// virtual void shape_and_test_foeppl_von_karman_at_knot(const unsigned& ipt,
//  Shape &psi, Shape& psi_b, Shape &test, Shape& test_b) const=0;

 /// \short Compute element residual Vector only (if flag=and/or element
 /// Jacobian matrix
 virtual void fill_in_generic_residual_contribution_foeppl_von_karman(
  Vector<double> &residuals, DenseMatrix<double> &jacobian,
  const unsigned& flag);

 /// Pointer to pressure function:
 PressureFctPt Pressure_fct_pt;

 /// Pointer to in plane forcing function (i.e. the shear force applied to
 /// the face)
 InPlaneForcingFctPt In_plane_forcing_fct_pt;

 /// Pointer to Poisson ratio, which this element cannot modify
 const double* Nu_pt;

 /// Pointer to global eta
 const double *Eta_pt;

 /// \short unsigned that holds the internal 'bubble' dofs the element has -
 // zero for Bell Elements and 3 for C1 curved elements
 unsigned Number_of_internal_dofs;

 /// \short unsigned that holds the number of types of degree of freedom at each
 // internal point that the element has zero for Bell Elements and 1
 // for C1 curved elements
 unsigned Number_of_internal_dof_types;

 /// Default value for physical constant: Poisson ratio. 
 static const double Default_Nu_Value;

 /// Default eta value so that we use 'natural' nondim and have no h dependence. 
 static const double Default_Eta_Value;

 /// Pointer to error metric
 ErrorMetricFctPt Error_metric_fct_pt;

 /// Pointer to error metric when we want multiple errors
 MultipleErrorMetricFctPt Multiple_error_metric_fct_pt;


 protected:

 /// Pointer to precomputed matrix that associates shape functions to monomials
 DenseMatrix<double> *Association_matrix_pt;
};


} //end namespace oomph
#endif

