#include "TGeoVGShape.h"
////////////////////////////////////////////////////////////////////////////
//                                                                        //
// TGeoVGShape - bridge class for using a VecGeom solid as TGeoShape.             //
//                                                                        //
////////////////////////////////////////////////////////////////////////////

#include "TGeoVGShape.h"

#include "volumes/PlacedVolume.h"
#include "volumes/UnplacedVolume.h"
#include "volumes/UnplacedBox.h"
#include "volumes/UnplacedTube.h"
#include "volumes/UnplacedCone.h"
#include "volumes/UnplacedParaboloid.h"
#include "volumes/UnplacedParallelepiped.h"
#include "volumes/UnplacedPolyhedron.h"
#include "volumes/UnplacedTrd.h"
#include "volumes/UnplacedOrb.h"
#include "volumes/UnplacedSphere.h"
#include "volumes/UnplacedBooleanVolume.h"
#include "volumes/UnplacedTorus2.h"
#include "volumes/UnplacedTrapezoid.h"
#include "volumes/UnplacedPolycone.h"
#include "volumes/UnplacedScaledShape.h"
#include "volumes/UnplacedGenTrap.h"
#include "TError.h"
#include "TGeoManager.h"
#include "TGeoMaterial.h"
#include "TGeoMedium.h"
#include "TGeoVolume.h"
#include "TGeoArb8.h"
#include "TGeoTube.h"
#include "TGeoCone.h"
#include "TGeoPara.h"
#include "TGeoParaboloid.h"
#include "TGeoTrd1.h"
#include "TGeoTrd2.h"
#include "TGeoPcon.h"
#include "TGeoPgon.h"
#include "TGeoSphere.h"
#include "TGeoBoolNode.h"
#include "TGeoCompositeShape.h"
#include "TGeoScaledShape.h"
#include "TGeoTorus.h"
#include "TGeoEltu.h"

//_____________________________________________________________________________
TGeoVGShape::TGeoVGShape(TGeoShape *shape,  vecgeom::cxx::VPlacedVolume *vgshape)
           :TGeoBBox(shape->GetName(), 0, 0, 0), fVGShape(vgshape), fShape(shape)
{
// Default constructor
   // Copy box parameters from the original ROOT shape
   const TGeoBBox *box = (const TGeoBBox*)shape;
   TGeoBBox::SetBoxDimensions(box->GetDX(), box->GetDY(), box->GetDZ());   
   memcpy(fOrigin, box->GetOrigin(), 3*sizeof(Double_t));
}

//_____________________________________________________________________________
TGeoVGShape::~TGeoVGShape()
{
// Destructor
   // Cleanup only the VecGeom solid, the ROOT shape is cleaned by TGeoManager 
   delete fVGShape;
}   

//_____________________________________________________________________________
TGeoVGShape *TGeoVGShape::Create(TGeoShape *shape)
{
// Factory creating TGeoVGShape from a Root shape. Returns nullptr if the 
// shape cannot be converted
   vecgeom::cxx::VPlacedVolume *vgshape = TGeoVGShape::CreateVecGeomSolid(shape);
   if (!vgshape) return nullptr;
   return ( new TGeoVGShape(shape, vgshape) );
}

//_____________________________________________________________________________
vecgeom::cxx::VPlacedVolume *TGeoVGShape::CreateVecGeomSolid(TGeoShape *shape)
{
// Conversion method to create VecGeom solid corresponding to TGeoShape
   // Call VecGeom TGeoShape->UnplacedSolid converter   
//   VUnplacedVolume *unplaced = RootGeoManager::Instance().Convert(shape);
   vecgeom::cxx::VUnplacedVolume *unplaced = Convert(shape);
   if (!unplaced) return nullptr;
   // We have to create a placed volume from the unplaced one to have access
   // to the navigation interface
   vecgeom::cxx::LogicalVolume *lvol = new vecgeom::cxx::LogicalVolume("", unplaced);
   return ( lvol->Place() );
}

//_____________________________________________________________________________
vecgeom::cxx::Transformation3D *TGeoVGShape::Convert(TGeoMatrix const *const geomatrix) {
// Convert a TGeoMatrix to a TRansformation3D
   Double_t const *const t = geomatrix->GetTranslation();
   Double_t const *const r = geomatrix->GetRotationMatrix();
   vecgeom::cxx::Transformation3D *const transformation =
        new vecgeom::cxx::Transformation3D(t[0], t[1], t[2], r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7], r[8]);
   return transformation;
}

//_____________________________________________________________________________
vecgeom::cxx::VUnplacedVolume* TGeoVGShape::Convert(TGeoShape const *const shape)
{
// Convert a TGeo shape to VUnplacedVolume, then creates a VPlacedVolume
   using namespace vecgeom::cxx;
   VUnplacedVolume *unplaced_volume = nullptr;

   // THE BOX
   if (shape->IsA() == TGeoBBox::Class()) {
      TGeoBBox const *const box = static_cast<TGeoBBox const *>(shape);
      unplaced_volume = new UnplacedBox(box->GetDX(), box->GetDY(), box->GetDZ());
   }

   // THE TUBE
   if (shape->IsA() == TGeoTube::Class()) {
      TGeoTube const *const tube = static_cast<TGeoTube const *>(shape);
      unplaced_volume = new UnplacedTube(tube->GetRmin(), tube->GetRmax(), tube->GetDz(), 0., kTwoPi);
   }

   // THE TUBESEG
   if (shape->IsA() == TGeoTubeSeg::Class()) {
      TGeoTubeSeg const *const tube = static_cast<TGeoTubeSeg const *>(shape);
      unplaced_volume = new UnplacedTube(tube->GetRmin(), tube->GetRmax(), tube->GetDz(), kDegToRad * tube->GetPhi1(),
                                         kDegToRad * (tube->GetPhi2() - tube->GetPhi1()));
   }

   // THE CONESEG
   if (shape->IsA() == TGeoConeSeg::Class()) {
      TGeoConeSeg const *const cone = static_cast<TGeoConeSeg const *>(shape);
      unplaced_volume =
          new UnplacedCone(cone->GetRmin1(), cone->GetRmax1(), cone->GetRmin2(), cone->GetRmax2(), cone->GetDz(),
                         kDegToRad * cone->GetPhi1(), kDegToRad * (cone->GetPhi2() - cone->GetPhi1()));
   }

   // THE CONE
   if (shape->IsA() == TGeoCone::Class()) {
      TGeoCone const *const cone = static_cast<TGeoCone const *>(shape);
      unplaced_volume = new UnplacedCone(cone->GetRmin1(), cone->GetRmax1(), cone->GetRmin2(), cone->GetRmax2(),
                                         cone->GetDz(), 0., kTwoPi);
   }

   // THE PARABOLOID
   if (shape->IsA() == TGeoParaboloid::Class()) {
      TGeoParaboloid const *const p = static_cast<TGeoParaboloid const *>(shape);
      unplaced_volume = new UnplacedParaboloid(p->GetRlo(), p->GetRhi(), p->GetDz());
   }

   // THE PARALLELEPIPED
   if (shape->IsA() == TGeoPara::Class()) {
      TGeoPara const *const p = static_cast<TGeoPara const *>(shape);
      unplaced_volume =
          new UnplacedParallelepiped(p->GetX(), p->GetY(), p->GetZ(), p->GetAlpha(), p->GetTheta(), p->GetPhi());
   }

   // Polyhedron/TGeoPgon
   if (shape->IsA() == TGeoPgon::Class()) {
      TGeoPgon const *pgon = static_cast<TGeoPgon const *>(shape);
      unplaced_volume = new UnplacedPolyhedron(pgon->GetPhi1(),   // phiStart
                                               pgon->GetDphi(),   // phiEnd
                                               pgon->GetNedges(), // sideCount
                                               pgon->GetNz(),     // zPlaneCount
                                               pgon->GetZ(),      // zPlanes
                                               pgon->GetRmin(),   // rMin
                                               pgon->GetRmax()    // rMax
                                               );
   }

   // TRD2
   if (shape->IsA() == TGeoTrd2::Class()) {
      TGeoTrd2 const *const p = static_cast<TGeoTrd2 const *>(shape);
      unplaced_volume = new UnplacedTrd(p->GetDx1(), p->GetDx2(), p->GetDy1(), p->GetDy2(), p->GetDz());
   }

   // TRD1
   if (shape->IsA() == TGeoTrd1::Class()) {
      TGeoTrd1 const *const p = static_cast<TGeoTrd1 const *>(shape);
      unplaced_volume = new UnplacedTrd(p->GetDx1(), p->GetDx2(), p->GetDy(), p->GetDz());
   }

   // TRAPEZOID
   if (shape->IsA() == TGeoTrap::Class()) {
      TGeoTrap const *const p = static_cast<TGeoTrap const *>(shape);
      unplaced_volume = new UnplacedTrapezoid(p->GetDz(), p->GetTheta() * kDegToRad, p->GetPhi() * kDegToRad, p->GetH1(),
                                              p->GetBl1(), p->GetTl1(), std::tan(p->GetAlpha1() * kDegToRad), p->GetH2(),
                                              p->GetBl2(), p->GetTl2(), std::tan(p->GetAlpha2() * kDegToRad));
   }

   // THE SPHERE | ORB
   if (shape->IsA() == TGeoSphere::Class()) {
      // make distinction
      TGeoSphere const *const p = static_cast<TGeoSphere const *>(shape);
      if (p->GetRmin() == 0. && p->GetTheta2() - p->GetTheta1() == 180. && p->GetPhi2() - p->GetPhi1() == 360.) {
         unplaced_volume = new UnplacedOrb(p->GetRmax());
      } else {
         unplaced_volume = new UnplacedSphere(p->GetRmin(), p->GetRmax(), p->GetPhi1() * kDegToRad,
                                             (p->GetPhi2() - p->GetPhi1()) * kDegToRad, p->GetTheta1(),
                                           (p->GetTheta2() - p->GetTheta1()) * kDegToRad);
      }
   }

   if (shape->IsA() == TGeoCompositeShape::Class()) {
      TGeoCompositeShape const *const compshape = static_cast<TGeoCompositeShape const *>(shape);
      TGeoBoolNode const *const boolnode = compshape->GetBoolNode();

      // need the matrix;
      Transformation3D const *lefttrans = Convert(boolnode->GetLeftMatrix());
      Transformation3D const *righttrans = Convert(boolnode->GetRightMatrix());
      // unplaced shapes
      VUnplacedVolume const *leftunplaced = Convert(boolnode->GetLeftShape());
      VUnplacedVolume const *rightunplaced = Convert(boolnode->GetRightShape());
      if (!leftunplaced || !rightunplaced) {
        // If one of the components cannot be converted, cleanup & return nullptr
        delete lefttrans;
        delete righttrans;
        delete leftunplaced;
        delete rightunplaced;
        return nullptr;
      }

      assert(leftunplaced != nullptr);
      assert(rightunplaced != nullptr);

      // the problem is that I can only place logical volumes
      VPlacedVolume *const leftplaced = (new LogicalVolume("inner_virtual", leftunplaced))->Place(lefttrans);

      VPlacedVolume *const rightplaced = (new LogicalVolume("inner_virtual", rightunplaced))->Place(righttrans);

      // now it depends on concrete type
      if (boolnode->GetBooleanOperator() == TGeoBoolNode::kGeoSubtraction) {
         unplaced_volume = new UnplacedBooleanVolume(kSubtraction, leftplaced, rightplaced);
      } else if (boolnode->GetBooleanOperator() == TGeoBoolNode::kGeoIntersection) {
         unplaced_volume = new UnplacedBooleanVolume(kIntersection, leftplaced, rightplaced);
      } else if (boolnode->GetBooleanOperator() == TGeoBoolNode::kGeoUnion) {
         unplaced_volume = new UnplacedBooleanVolume(kUnion, leftplaced, rightplaced);
      }
   }

   // THE TORUS
   if (shape->IsA() == TGeoTorus::Class()) {
      // make distinction
      TGeoTorus const *const p = static_cast<TGeoTorus const *>(shape);
      unplaced_volume =
          new UnplacedTorus2(p->GetRmin(), p->GetRmax(), p->GetR(), p->GetPhi1() * kDegToRad, p->GetDphi() * kDegToRad);
   }

   // THE POLYCONE
   if (shape->IsA() == TGeoPcon::Class()) {
      TGeoPcon const *const p = static_cast<TGeoPcon const *>(shape);
      unplaced_volume = new UnplacedPolycone(p->GetPhi1() * kDegToRad, p->GetDphi() * kDegToRad, p->GetNz(), p->GetZ(),
                                             p->GetRmin(), p->GetRmax());
   }

   // THE SCALED SHAPE
   if (shape->IsA() == TGeoScaledShape::Class()) {
      TGeoScaledShape const *const p = static_cast<TGeoScaledShape const *>(shape);
      // First convert the referenced shape
      VUnplacedVolume *referenced_shape = Convert(p->GetShape());
      if (!referenced_shape) return nullptr;
      const double *scale_root = p->GetScale()->GetScale();
      unplaced_volume = new UnplacedScaledShape(referenced_shape, scale_root[0], scale_root[1], scale_root[2]);
   }

   // THE ELLIPTICAL TUBE AS SCALED TUBE
   if (shape->IsA() == TGeoEltu::Class()) {
      TGeoEltu const *const p = static_cast<TGeoEltu const *>(shape);
      // Create the corresponding unplaced tube, with:
      //   rmin=0, rmax=A, dz=dz, which is scaled with (1., A/B, 1.)
      UnplacedTube *tubeUnplaced = new UnplacedTube(0, p->GetA(), p->GetDZ(), 0, kTwoPi);
      unplaced_volume = new UnplacedScaledShape(tubeUnplaced, 1., p->GetB() / p->GetA(), 1.);
   }

   // THE ARB8
   if (shape->IsA() == TGeoArb8::Class() || shape->IsA() == TGeoGtra::Class()) {
      TGeoArb8 *p = (TGeoArb8 *)(shape);
      // Create the corresponding GenTrap
      std::vector<Vector3D<Precision>> vertexlist;
      const double *vertices = p->GetVertices();
      Precision verticesx[8], verticesy[8];
      for (auto ivert = 0; ivert < 8; ++ivert) {
         verticesx[ivert] = vertices[2 * ivert];
         verticesy[ivert] = vertices[2 * ivert + 1];
      }
      unplaced_volume = new UnplacedGenTrap(verticesx, verticesy, p->GetDz());
   }

   // New volumes should be implemented here...
   if (!unplaced_volume) {
      printf("Unsupported shape for ROOT shape \"%s\" of type %s. "
             "Using ROOT implementation.\n",
             shape->GetName(), shape->ClassName());
      return nullptr;
   }

   return ( unplaced_volume );
}

//_____________________________________________________________________________
void TGeoVGShape::ComputeBBox()
{
// Compute bounding box.
  fShape->ComputeBBox();
}   

//_____________________________________________________________________________
Double_t TGeoVGShape::Capacity() const
{
// Returns analytic capacity of the solid
   return fVGShape->Capacity();
}

//_____________________________________________________________________________
void TGeoVGShape::ComputeNormal(const Double_t *point, const Double_t */*dir*/, Double_t *norm)
{
// Normal computation.
   vecgeom::cxx::Vector3D<Double_t> vnorm;
   fVGShape->Normal(vecgeom::cxx::Vector3D<Double_t>(point[0], point[1], point[2]), vnorm);
   norm[0] = vnorm.x(); norm[1] = vnorm.y(), norm[2] = vnorm.z();
}
   
//_____________________________________________________________________________
Bool_t TGeoVGShape::Contains(const Double_t *point) const
{
// Test if point is inside this shape.
   return ( fVGShape->Contains(vecgeom::cxx::Vector3D<Double_t>(point[0], point[1], point[2])) );
}

//_____________________________________________________________________________
Double_t TGeoVGShape::DistFromInside(const Double_t *point, const Double_t *dir, Int_t /*iact*/, 
                                   Double_t step, Double_t * /*safe*/) const
{
   Double_t dist = fVGShape->DistanceToOut(vecgeom::cxx::Vector3D<Double_t>(point[0], point[1], point[2]),
                                    vecgeom::cxx::Vector3D<Double_t>(dir[0], dir[1], dir[2]), step);
   return ( (dist < 0.)? 0. : dist );
}

//_____________________________________________________________________________
Double_t TGeoVGShape::DistFromOutside(const Double_t *point, const Double_t *dir, Int_t /*iact*/, 
                                   Double_t step, Double_t * /*safe*/) const
{
   Double_t dist = fVGShape->DistanceToIn(vecgeom::cxx::Vector3D<Double_t>(point[0], point[1], point[2]),
                                    vecgeom::cxx::Vector3D<Double_t>(dir[0], dir[1], dir[2]), step);
   return ( (dist < 0.)? 0. : dist );
}

//_____________________________________________________________________________
Double_t TGeoVGShape::Safety(const Double_t *point, Bool_t in) const
{
   Double_t safety =  (in) ? fVGShape->SafetyToOut(vecgeom::cxx::Vector3D<Double_t>(point[0], point[1], point[2]))
                           : fVGShape->SafetyToIn(vecgeom::cxx::Vector3D<Double_t>(point[0], point[1], point[2]));
   return ( (safety < 0.)? 0. : safety );
   
}

//_____________________________________________________________________________
void TGeoVGShape::InspectShape() const
{
// Print info about the VecGeom solid
   fVGShape->GetUnplacedVolume()->Print();
   printf("\n");
}
