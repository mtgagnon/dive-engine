#ifdef c
#ifndef MAPHELPER_H
#define MAPHELPER_H

/* MapHelper.h : Utility header containing convenient data structures for homework #3 */
/* (mainly so that you don't need to type or copy-paste directly from the spec) */

#include <string>
#include <vector>
#include "glm/glm.hpp" // Student : You need to get glm added to your project source code or this line will fail.
                       // Search for your IDE's "include / header directories" option (it's called something different everywhere)
                       // Ensure you have the glm folder listed so your compiler can actually find these glm headers.

struct Actor
{
public:
    std::string actor_name;
    char view;
    glm::ivec2 position;
    glm::ivec2 velocity;
    bool blocking;
    std::string nearby_dialogue;
    std::string contact_dialogue;

    Actor(std::string actor_name, char view, glm::ivec2 position, glm::ivec2 initial_velocity,
        bool blocking, std::string nearby_dialogue, std::string contact_dialogue)
        : actor_name(actor_name), view(view), position(position), velocity(initial_velocity), blocking(blocking), nearby_dialogue(nearby_dialogue), contact_dialogue(contact_dialogue) {}

    Actor() {}
};

const std::string game_start_message = "January 2024- East Lansing, MI.\n\
The nation's #1 game development program grows anxious.\n\
\n\
Despite their exceptional performance and rankings (seriously, you should consider their graduate school), the Spartans of Michigan State University can't help but feel a storm brewing in Ann Arbor. \"The Wolverines are putting together quite a program, aren't they?\" \"Those UMich students - their programming abilities are exceptional, and they're stealing all the gamedev internships!\", \"if their scene continues this momentum, we may be in trouble...\"\n\
\n\
Having stormed the University of Michigan campus twice before - and having been repelled twice before (check out the Crisis Wolverine games)--the Spartans were eager to make their third try a lucky one...\n\
\n\
The struggle began one Sunday at 3am. With most students and faculty sleeping, the Spartans struck, taking over the Duderstadt Center with Spartan speed - their goal ? To destroy the 498 autograder and ruin the new game engine course.\n\
\n\
Fortunately, you (p) were working on homework late into the night in the VisStudio.When the spartans (s) burst through the lab door, you quickly grabbed the autograder (a raspberry pie 4) from the front desk and hid beneath a table as the lights went out. Despite your late - night fatigue, you begin feeling your way past the walls (b) and through the halls. Can you escape, or will Michigan gamedev's newest course crash and burn?\n\
\n\
G A M E - S T A R T";

const std::string game_over_bad_message = "Exhausted by the chase, the darkness begins to swirl around you.\n\
\n\
Suddenly the floor greets you, followed by pure silence. You've fallen asleep.It is 3am after all, and your physiology has made an executive decision.\n\
\n\
You wake up to bright lights- the Duderstadt Center is now lively and back-to-business. People keep looking at you funny however, as if the Spartans from last night have written something on your forehead. You're too afraid to ask or even check for yourself. What's worse, you notice the autograder is no longer in your possession.\n\
\n\
Upon arriving home, you find an email from the Professor sitting ominously in your inbox.\n\
\n\
\"Hi Class,\n\
\n\
I want to apologize- I messed up and left my only raspberry pi in the VisStudio last night. You know- the one with the autograder on it. Unfortunately, I made an even greater error in forgetting to use proper version control, so I'm afraid the autograder software is toast. I know this is all devastating to you, but I'll have to give extensions to everyone, and make the homework much easier.\"\n\
\n\
What a bitter-sweet win for the spartans. While you end up enjoying your extension, you aren't able to learn as much given the tight class schedule and cut course content. You feel a bit cheated, and live in the most mild state terror, contemplating how the spartans could disrupt your other courses, EECS 494 and EECS 480 (XR).\n\
\n\
Y O U L O S E\n\
G A M E O V E R";

const std::string game_over_good_message = "As you near the door, the air itself flickers to life with angry voices.\n\
\n\
\"Hey! They've got the autograder! Get em!\", \"Aww shucks they totes beat us.\", \"Go green baby! Johnny Foreshadow sucks!\", \"Heeeey what's an autograder anywaysss??\"\n\
\n\
You're a bit confused by what they're saying, and make a mental note to play the Crisis Wolverine games later. You decide to leave, and burst through the door with the might of a late-night EECS student.\n\
\n\
The crisp cool January air greets you in force-- you are free, and the course is saved! You briefly consider applying some of those EECS 388 skills to the autograder before returning it to Professor Yarger... But nah. Game Engine Architecture is tough, but you've got this!\n\
\n\
Y O U W I N\n\
G A M E O V E R";

const std::string initial_render = "\
       s     \n\
bbbb    s    \n\
f  bbbbb s   \n\
         b   \n\
      p  b   \n\
         b   \n\
bbbbbbbbb    \n\
             \n\
          b  ";


const int HARDCODED_MAP_WIDTH = 50;
const int HARDCODED_MAP_HEIGHT = 26;
const char hardcoded_map[HARDCODED_MAP_HEIGHT][HARDCODED_MAP_WIDTH+1] = { // Know why we need "+1"? Hint : These are c-style strings.
    "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb",
    "bb    b       b                   b        b    bb",
    "bb            b                   b             bb",
    "bb    bbbbbbbbbbbbbb              bbbbbbbbbb    bb",
    "bb            b      b            b             bb",
    "bb    b       b       b   bb      b        b    bb",
    "bb    b  bb bbb     bb      bb    bbbbbbbbbb    bb",
    "bb    b  b    b   bb          bb           b    bb",
    "bb    b  b   b  bb        bb    bb         b    bb",
    "bbb   bbbb  bbbb                  bbbbbbbbbb   bbb",
    "b                   bb                           b",
    "b                                                b",
    "bbb    bbbb  bbbb          bb    bbbbbbbbbbbb  bbb",
    "bb       b      bbbbb        bbb           b    bb",
    "bb       b            b    b               b    bb",
    "bb       b            b    b               b    bb",
    "bb       b            b    b               b    bb",
    "bb       bbbbbbbbbbbbb      bbbbbbbbbbbbbbbb    bb",
    "bb                                              bb",
    "bb        bb           b  b         bb          bb",
    "bb                    b    b            bb      bb",
    "bbbbb    bb           b    b                   bbb",
    "bb   bb                bbbb      bb      bb      b",
    "bb     bb    bb                      bb          b",
    "bbbbbbbbbbbbbbbb   bbbbbbbbbbbb   bbbbbbbbbbbbbbbb",
    "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
};

inline std::vector<Actor> hardcoded_actors = {
    Actor(
        "spartan1", // Actor name
        's', // view
        glm::ivec2(21, 12), // starting position
        glm::ivec2(0, 0), // starting velocity
        true, // blocking?
        "Hey, you! (the spartan throws a fake plastic spear at you. It bounces off but kinda hurts your ego a bit. [health down])", // nearby_dialogue
        "Gotcha! (the spartan is right next to you, and pokes you with a fake plastic spear. They're really into it and having a good time, and we can't be having that. [health down])" // contact_dialogue
    ),
    Actor(
        "spartan2",
        's',
        glm::ivec2(22, 13),
        glm::ivec2(0, 0),
        true,
        "Hey, you! (the spartan throws a fake plastic spear at you. It bounces off but kinda hurts your ego a bit. [health down])",
        "Gotcha! (the spartan is right next to you, and pokes you with a fake plastic spear. They're really into it and having a good time, and we can't be having that. [health down])"
    ),
    Actor(
        "spartan3",
        's',
        glm::ivec2(20, 11),
        glm::ivec2(0, 0),
        true,
        "Hey, you! (the spartan throws a fake plastic spear at you. It bounces off but kinda hurts your ego a bit. [health down])",
        "Gotcha! (the spartan is right next to you, and pokes you with a fake plastic spear. They're really into it and having a good time, and we can't be having that. [health down])"
    ),
    Actor(
        "spartan4",
        's',
        glm::ivec2(18, 9),
        glm::ivec2(0, 0),
        true,
        "Hey, you! (the spartan throws a fake plastic spear at you. It bounces off but kinda hurts your ego a bit. [health down])",
        "Gotcha! (the spartan is right next to you, and pokes you with a fake plastic spear. They're really into it and having a good time, and we can't be having that. [health down])"
    ),
    Actor(
        "spartan5",
        's',
        glm::ivec2(18, 10),
        glm::ivec2(-1, 0),
        true,
        "Hey, you! (the spartan throws a fake plastic spear at you. It bounces off but kinda hurts your ego a bit. [health down])",
        "Gotcha! (the spartan is right next to you, and pokes you with a fake plastic spear. They're really into it and having a good time, and we can't be having that. [health down])"
    ),
    Actor(
        "spartan6",
        's',
        glm::ivec2(4, 11),
        glm::ivec2(1, 0),
        true,
        "Hey, you! (the spartan throws a fake plastic spear at you. It bounces off but kinda hurts your ego a bit. [health down])",
        "Gotcha! (the spartan is right next to you, and pokes you with a fake plastic spear. They're really into it and having a good time, and we can't be having that. [health down])"
    ),
    Actor(
        "sparty",
        'S',
        glm::ivec2(12, 22),
        glm::ivec2(1, 0),
        false,
        "Oh no... it's Sparty himself! His sinister unflinching eyes scan the darkness in your direction. You'd better be extra careful!)",
        "(you bump into sparty and he falls onto you clumsily. It's all very embarrassing and you can feel the life squeezing out of you) [game over]"
    ),
    Actor(
        "wolverine1",
        'w',
        glm::ivec2(11, 15),
        glm::ivec2(0, 0),
        false,
        "(One of your wolverine classmates is nearby) \"Hey-- be careful! Some spartans are sneaking around over there. Stay far away and slip past if you can... It looks like some of the other students are panicking, which I told them to not do! And now they're lost in the darkness. Look for their ('f') icon and go see if you can calm them down. Then, get out of there and go find Professor Yarger!\"",
        "(you're standing on your friend, but they don't seem to mind. They are a good friend.)"
    ),
        Actor(
        "494 student",
        '4',
        glm::ivec2(10, 16),
        glm::ivec2(0, 0),
        true,
        "(There's an EECS 494 student working on her laptop nearby. She looks very busy and is doesn't want to talk.)",
        "()"
    ),
    Actor(
        "exit_door_2",
        'd',
        glm::ivec2(48, 22),
        glm::ivec2(0, 0),
        false,
        "(An exit door is nearby)",
        "(you open the door) [you win]"
    ),
    Actor(
        "exit_door_1",
        'd',
        glm::ivec2(48, 23),
        glm::ivec2(0, 0),
        false,
        "(An exit door is nearby)",
        "(you open the door) [you win]"
    ),
    Actor(
        "friend1",
        'f',
        glm::ivec2(11, 8),
        glm::ivec2(0, 0),
        true,
        "H-hey (you spot a nearby friend in the darkness). Thank you for f-finding me. I'll sit here and rest a bit. [score up]",
        ""
    ),
    Actor(
        "friend2",
        'f',
        glm::ivec2(13, 13),
        glm::ivec2(0, 0),
        true,
        "O-oh (you spot a nearby friend in the darkness). I'm glad I'm not a-alone... I'll sit here and r-rest a bit. Thanks. [score up]",
        ""
    ),
    Actor(
        "player",            // actor name
        'p',                // view
        glm::ivec2(19, 15),    // starting velocity
        glm::ivec2(0, 0),    // starting velocity
        false,                // blocking?
        "",                    // nearby dialogue
        ""                    // made-contact dialogue
    ),
};

#endif
#endif
