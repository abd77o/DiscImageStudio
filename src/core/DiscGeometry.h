// DiscGeometry.h - spiral-track geometry of a CD (CLV, 1.2 m/s, 75 sectors/s).
// Closed-form mapping between "position along the track" and (radius, angle).
#pragma once
#include <cmath>
#include <cstdint>

namespace dis {

const double kPi = 3.14159265358979323846;

struct DiscGeometry {
    static const int kSectorBytes = 2352;   // raw main-channel bytes per sector

    double programStartMm = 25.0;           // radius where sector 0 of the image area starts
    double programEndMm   = 58.0;           // physical limit of the programme area
    double trackPitchMm   = 0.0016;         // 1.6 um
    double sectorLengthMm = 16.0;           // 1.2 m/s / 75 sectors/s

    double mmPerByte() const { return sectorLengthMm / kSectorBytes; }

    // Spiral: r^2 = r0^2 + p*s/pi   (s = length travelled along the track)
    double radiusAtLength(double sMm) const {
        return std::sqrt(programStartMm * programStartMm + trackPitchMm * sMm / kPi);
    }
    double lengthAtRadius(double rMm) const {
        return kPi * (rMm * rMm - programStartMm * programStartMm) / trackPitchMm;
    }
    // Total rotation (radians) since the start of the spiral: one turn per track pitch.
    double angleAtRadius(double rMm) const {
        return 2.0 * kPi * (rMm - programStartMm) / trackPitchMm;
    }
};

} // namespace dis
