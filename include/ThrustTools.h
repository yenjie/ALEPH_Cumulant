#ifndef ALEPH_CUMULANT_THRUSTTOOLS_H
#define ALEPH_CUMULANT_THRUSTTOOLS_H

#include <algorithm>
#include <cmath>
#include <vector>

#include "TMath.h"
#include "TVector3.h"

namespace AlephCumulant
{
   inline bool IsChargedPwflag(short pwflag)
   {
      return pwflag >= 0 && pwflag <= 2;
   }

   inline TVector3 SafeUnit(TVector3 v)
   {
      if (v.Mag2() <= 0.0)
         return TVector3(0.0, 0.0, 0.0);
      return v.Unit();
   }

   inline TVector3 ComputeThrustAxis(const std::vector<TVector3> &momenta)
   {
      if (momenta.empty())
         return TVector3(0.0, 0.0, 0.0);

      double totalMomentum = 0.0;
      for (const TVector3 &p : momenta)
         totalMomentum += p.Mag();
      if (totalMomentum <= 0.0)
         return TVector3(0.0, 0.0, 0.0);

      TVector3 bestAxis;
      double bestProjection = -1.0;
      const int count = static_cast<int>(momenta.size());

      for (int i = 1; i < count; ++i)
      {
         for (int j = 0; j < i; ++j)
         {
            const TVector3 cross = momenta[i].Cross(momenta[j]);
            if (cross.Mag2() <= 0.0)
               continue;

            TVector3 signedSum(0.0, 0.0, 0.0);
            for (int k = 0; k < count; ++k)
            {
               if (k == i || k == j)
                  continue;
               signedSum += (momenta[k].Dot(cross) >= 0.0) ? momenta[k] : -momenta[k];
            }

            const TVector3 candidates[4] = {
               signedSum - momenta[j] - momenta[i],
               signedSum - momenta[j] + momenta[i],
               signedSum + momenta[j] - momenta[i],
               signedSum + momenta[j] + momenta[i],
            };

            for (const TVector3 &candidate : candidates)
            {
               const TVector3 axis = SafeUnit(candidate);
               if (axis.Mag2() <= 0.0)
                  continue;

               double projection = 0.0;
               for (const TVector3 &p : momenta)
                  projection += std::abs(p.Dot(axis));
               if (projection > bestProjection)
               {
                  bestProjection = projection;
                  bestAxis = axis;
               }
            }
         }
      }

      if (bestAxis.Mag2() <= 0.0)
         return SafeUnit(momenta.front());
      return bestAxis;
   }

   inline double PtFromAxis(const TVector3 &axis, const TVector3 &p)
   {
      const TVector3 unit = SafeUnit(axis);
      if (unit.Mag2() <= 0.0)
         return 0.0;
      return p.Perp(unit);
   }

   inline double EtaFromAxis(const TVector3 &axis, const TVector3 &p)
   {
      const TVector3 unit = SafeUnit(axis);
      if (unit.Mag2() <= 0.0 || p.Mag2() <= 0.0)
         return 0.0;

      const double theta = p.Angle(unit);
      const double tanHalfTheta = std::tan(theta / 2.0);
      if (tanHalfTheta <= 0.0)
         return (theta < TMath::Pi() / 2.0) ? 99.0 : -99.0;

      return std::max(-99.0, std::min(99.0, -std::log(tanHalfTheta)));
   }

   inline double PhiFromAxis(const TVector3 &axis, const TVector3 &p)
   {
      const TVector3 axisUnit = SafeUnit(axis);
      if (axisUnit.Mag2() <= 0.0 || p.Mag2() <= 0.0)
         return 0.0;

      TVector3 transverse = p - p.Dot(axisUnit) * axisUnit;
      if (transverse.Mag2() <= 0.0)
         return 0.0;
      transverse = transverse.Unit();

      const TVector3 beam(0.0, 0.0, 1.0);
      TVector3 phiOrigin = axisUnit.Cross(axisUnit.Cross(beam));
      if (phiOrigin.Mag2() <= 1e-12)
         phiOrigin = axisUnit.Cross(TVector3(1.0, 0.0, 0.0));
      if (phiOrigin.Mag2() <= 1e-12)
         phiOrigin = axisUnit.Cross(TVector3(0.0, 1.0, 0.0));
      phiOrigin = phiOrigin.Unit();

      const double cosPhi = std::max(-1.0, std::min(1.0, phiOrigin.Dot(transverse)));
      const double phi = std::acos(cosPhi);
      return (phiOrigin.Cross(transverse).Dot(axisUnit) >= 0.0) ? phi : -phi;
   }
}

#endif

