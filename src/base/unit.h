/****************************************************************************
** Copyright (c) 2021, Fougue Ltd. <http://www.fougue.pro>
** All rights reserved.
** See license at https://github.com/fougue/mayo/blob/master/LICENSE.txt
****************************************************************************/

#pragma once

namespace Mayo
{

enum class Unit
{
    None,

    // Base
    Length,                   // Meter(m)
    Mass,                     // Kilogram(kg)
    Time,                     // Second(s)
    ElectricCurrent,          // Ampere(A)
    ThermodynamicTemperature, // Kelvin(K)
    AmountOfSubstance,        // Mole(mol)
    LuminousIntensity,        // Candela(cd)
    Angle,                    // Radian(rad)

    // Derived
    Area,         // m²
    Volume,       // m^3
    Velocity,     // m/s
    Acceleration, // m/s²
    Density,      // kg/m^3
    Pressure      // kg/m.s² (or N/m²)
};

enum LengthUnit
{
    // SI
    Nanometer,
    Micrometer,
    Millimeter,
    Centimeter,
    Decimeter,
    Meter,
    Kilometer,
    NauticalMile,
    // Imperial UK
    Thou,
    Inch,
    Link,
    Foot,
    Yard,
    Rod,
    Chain,
    Furlong,
    Mile,
    League
};

enum AngleUnit
{
    Radian,
    Degree
};

enum AreaUnit
{
    // SI
    SquareMillimeter,
    SquareCentimeter,
    SquareMeter,
    SquareKilometer,
    // Imperial UK
    SquareInch,
    SquareFoot,
    SquareYard,
    SquareMile
};

enum VolumeUnit
{
    // SI
    CubicMillimeter,
    CubicCentimeter,
    CubicMeter,
    // Imperial UK
    CubicInch,
    CubicFoot,
    // Others
    Liter,
    ImperialGallon,
    USGallon
};
} // namespace Mayo
