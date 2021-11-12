#include "MoviePlayerImpl.hpp"

MoviePlayer *MoviePlayer::GetPlayer()
{
    return new MoviePlayerImpl();
}