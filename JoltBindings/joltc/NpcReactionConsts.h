#ifndef NPC_REACTION_CONSTS_H_
#define NPC_REACTION_CONSTS__H_ 1

#include "BlackSaber1NpcReaction.h"
#include "BlackShooter1NpcReaction.h"
#include "BlackThrower1NpcReaction.h"
#include "BlackSaber2NpcReaction.h"
#include "BlackShooter2NpcReaction.h"
#include "ShieldGuard1NpcReaction.h"
#include "Bat1NpcReaction.h"
#include "Wolverine1NpcReaction.h"
#include "BlackSaber1TestWithVisionNpcReaction.h"
#include "Wolverine1TestWithVisionNpcReaction.h"
#include "BlackThrower1TestWithVisionNpcReaction.h"
#include "Bat1TestWithVisionNpcReaction.h"
#include <map>

extern JOLTC_EXPORT std::unordered_map<uint32_t, BaseNpcReaction*> globalNpcReactionMap;

#endif
