//
// Author: Marcin Kosiba
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//


#ifndef GAUSS_MARKOV_MOBILITY_H
#define GAUSS_MARKOV_MOBILITY_H

#include "INETDefs.h"

#include "LineSegmentsMobilityBase.h"

namespace inet {


/**
 * @brief Gauss Markov movement model. See NED file for more info.
 *
 * @author Marcin Kosiba
 */
class INET_API GaussMarkovMobility : public LineSegmentsMobilityBase
{
  protected:
    double speed;          ///< speed of the host
    double angle;          ///< angle of linear motion
    double alpha;          ///< alpha parameter
    int margin;            ///< margin at which the host gets repelled from the border
    double speedMean;      ///< speed mean
    double angleMean;      ///< angle mean
    double variance;       ///< variance

  protected:
    virtual int numInitStages() const { return NUM_INIT_STAGES; }

    /** @brief Initializes mobility model parameters.*/
    virtual void initialize(int stage);

    /** @brief If the host is too close to the border it is repelled */
    void preventBorderHugging();

    /** @brief Move the host*/
    virtual void move();

    /** @brief Calculate a new target position to move to. */
    virtual void setTargetPosition();

  public:
    GaussMarkovMobility();
};

}


#endif
