#pragma once

#include <array>
#include <string>

namespace wumpus {

const int num_rooms = 20;
const int connections_per_room = 3;
const std::array<std::array<int, connections_per_room>, num_rooms> room_connections{
    {{{1, 4, 5}},    {{2, 0, 7}},    {{3, 1, 9}},    {{4, 2, 11}},
     {{0, 3, 13}},   {{6, 14, 0}},   {{7, 5, 15}},   {{8, 6, 1}},
     {{9, 7, 16}},   {{10, 8, 2}},   {{11, 9, 17}},  {{12, 10, 3}},
     {{13, 11, 18}}, {{14, 12, 4}},  {{5, 13, 19}},  {{16, 19, 6}},
     {{17, 15, 8}},  {{18, 16, 10}}, {{19, 17, 12}}, {{15, 18, 14}}}};

const int num_bats = 2;
const int num_pits = 2;

const int arrow_range = 3;
const int num_arrows = 5;

struct Room
{
    int number;
    bool wumpus{false};
    bool bat{false};
    bool pit{false};
    std::array<Room*, connections_per_room> adjacent_rooms;

    Room() : number{}
    {
    }

    explicit Room(int number) : number{number}
    {
    }
};

enum class Game_state
{
    none,
    player_eaten,
    player_fell,
    player_shot,
    wumpus_dead,
    player_quit
};

class Game
{
public:
    static const std::string wumpus_adjacent_message;
    static const std::string bat_adjacent_message;
    static const std::string pit_adjacent_message;
    static const std::string wumpus_dead_message;
    static const std::string player_eaten_message;
    static const std::string player_dropped_in_random_room_message;
    static const std::string player_fell_message;
    static const std::string player_shot_message;
    static const std::string player_quit_message;
    static const std::string wumpus_moves_message;

    static std::string game_info();

    Game(std::ostream& out);

    // Game Actions API.
    void init_hunt();

    bool is_hunt_over() const;
    void inform_player_of_hazards();
    void end_hunt();

    bool can_move(int target) const;
    void move(int target);
    bool can_shoot(const std::array<int, arrow_range>& targets) const;
    void shoot(const std::array<int, arrow_range>& targets);
    void quit();

    // Game State API.
    const std::array<Room, num_rooms>& get_rooms() const;
    const Room* get_player_room() const;
    Game_state get_game_state() const;
    int get_arrows() const;

private:
    std::ostream& out;

    std::array<Room, num_rooms> rooms;
    Room* player_room{nullptr};
    Room* wumpus_room{nullptr};

    Game_state state{Game_state::none};

    int arrows{num_arrows};

    void reset_rooms();
    void shuffle_room_numbers();
    void sort_adjacent_rooms();
    void place_player_and_hazards();

    bool target_is_adjacent(int target) const;
    void check_room_hazards();
    Room* get_next_room_for_arrow_flight(
        const Room* previous_room, const Room* current_room, int target) const;
    void move_wumpus();
};
}
