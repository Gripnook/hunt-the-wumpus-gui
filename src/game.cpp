#include "game.h"

#include <vector>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <stdexcept>

#include "random.h"

namespace wumpus {

const std::string Game::wumpus_adjacent_message = "You smell the wumpus!";
const std::string Game::bat_adjacent_message = "You hear flapping!";
const std::string Game::pit_adjacent_message = "You feel a breeze!";
const std::string Game::wumpus_dead_message =
    "Congratulations, you have slain the wumpus!";
const std::string Game::player_eaten_message =
    "You have been eaten by the wumpus!";
const std::string Game::player_dropped_in_random_room_message =
    "You are carried away by a bat!";
const std::string Game::player_fell_message =
    "You have fallen into a bottomless pit!";
const std::string Game::player_shot_message =
    "You have been hit with your own arrow!";
const std::string Game::player_quit_message = "You flee the cave!";
const std::string Game::wumpus_moves_message =
    "You hear the sound of the wumpus moving!";

std::string Game::game_info()
{
    std::stringstream ss;
    ss << "Welcome to Hunt the Wumpus." << std::endl
       << "Your job is to slay the wumpus living in the cave using bow and "
          "arrow."
       << std::endl
       << "Each of the " << num_rooms << " rooms is connected to "
       << connections_per_room << " other rooms by dark tunnels." << std::endl
       << "In addition to the wumpus, the cave has two hazards: bottomless "
          "pits and"
       << std::endl
       << "giant bats. If you enter a room with a bottomless pit, it's the "
          "end of the"
       << std::endl
       << "game for you. If you enter a room with a bat, the bat picks you up "
          "and"
       << std::endl
       << "drops you into another room. If you enter the room with the wumpus "
          "or he"
       << std::endl
       << "enters yours, he eats you. There are " << num_pits << " pits and "
       << num_bats << " bats in the cave." << std::endl
       << "When you enter a room you will be told if a hazard is nearby:"
       << std::endl
       << "    \"" << wumpus_adjacent_message << "\": It's in an adjacent room."
       << std::endl
       << "    \"" << pit_adjacent_message
       << "\": One of the adjacent rooms is a bottomless pit." << std::endl
       << "    \"" << bat_adjacent_message
       << "\": A giant bat is in an adjacent room." << std::endl;
    return ss.str();
}

Game::Game(std::ostream& out) : out{out}
{
    for (int i = 0; i < num_rooms; ++i)
    {
        rooms[i] = Room(i + 1);
        for (int j = 0; j < connections_per_room; ++j)
            rooms[i].adjacent_rooms[j] = &rooms[room_connections[i][j]];
    }
}

void Game::init_hunt()
{
    state = Game_state::none;
    arrows = num_arrows;
    reset_rooms();
    shuffle_room_numbers();
    place_player_and_hazards();
}

void Game::reset_rooms()
{
    for (Room& room : rooms)
    {
        room.wumpus = false;
        room.bat = false;
        room.pit = false;
    }
}

void Game::shuffle_room_numbers()
{
    for (int i = 0; i < num_rooms; ++i)
        std::swap(rooms[i].number, rooms[random(i, num_rooms)].number);
    sort_adjacent_rooms(); // This is necessary to eliminate patterns in the
                           // display of adjacent rooms.
}

void Game::sort_adjacent_rooms()
{
    for (Room& room : rooms)
    {
        std::sort(
            std::begin(room.adjacent_rooms),
            std::end(room.adjacent_rooms),
            [](Room* first_room, Room* second_room) {
                return first_room->number < second_room->number;
            });
    }
}

void Game::place_player_and_hazards()
{
    std::vector<int> random_locations;
    for (int i = 0; i < 2 + num_bats + num_pits; ++i)
        random_locations.push_back(random(0, num_rooms, random_locations));

    int index = 0;
    player_room = &rooms[random_locations[index++]];
    wumpus_room = &rooms[random_locations[index++]];
    ;
    wumpus_room->wumpus = true;
    while (index < 2 + num_bats)
        rooms[random_locations[index++]].bat = true;
    while (index < 2 + num_bats + num_pits)
        rooms[random_locations[index++]].pit = true;
}

bool Game::is_hunt_over() const
{
    return state != Game_state::none;
}

void Game::inform_player_of_hazards()
{
    bool wumpus = false, bat = false, pit = false;
    for (const Room* room : player_room->adjacent_rooms)
    {
        if (room->wumpus)
            wumpus = true;
        if (room->bat)
            bat = true;
        if (room->pit)
            pit = true;
    }
    if (wumpus)
        out << wumpus_adjacent_message << std::endl;
    if (bat)
        out << bat_adjacent_message << std::endl;
    if (pit)
        out << pit_adjacent_message << std::endl;
}

void Game::end_hunt()
{
    switch (state)
    {
    case Game_state::player_eaten:
        out << player_eaten_message << std::endl;
        break;
    case Game_state::player_fell:
        out << player_fell_message << std::endl;
        break;
    case Game_state::player_shot:
        out << player_shot_message << std::endl;
        break;
    case Game_state::wumpus_dead:
        out << wumpus_dead_message << std::endl;
        break;
    case Game_state::player_quit:
        out << player_quit_message << std::endl;
        break;
    default:
        throw std::logic_error("Invalid end of game state");
    }
}

bool Game::can_move(int target) const
{
    return target_is_adjacent(target);
}

void Game::move(int target)
{
    for (Room* room : player_room->adjacent_rooms)
        if (room->number == target)
            player_room = room;
    check_room_hazards();
}

bool Game::can_shoot(const std::array<int, arrow_range>& targets) const
{
    return arrows > 0 && target_is_adjacent(targets[0]);
}

void Game::shoot(const std::array<int, arrow_range>& targets)
{
    --arrows;
    Room* room = player_room;
    Room* previous_room = nullptr;
    for (int i = 0; i < arrow_range; ++i)
    {
        Room* next_previous_room = room;
        room = get_next_room_for_arrow_flight(previous_room, room, targets[i]);
        previous_room = next_previous_room;
        if (room->wumpus)
        {
            state = Game_state::wumpus_dead;
            return;
        }
        if (room->number == player_room->number)
        {
            state = Game_state::player_shot;
            return;
        }
    }
    move_wumpus();
}

void Game::quit()
{
    state = Game_state::player_quit;
}

bool Game::target_is_adjacent(int target) const
{
    for (const Room* room : player_room->adjacent_rooms)
        if (room->number == target)
            return true;
    return false;
}

void Game::check_room_hazards()
{
    while (true)
    {
        if (player_room->wumpus)
        {
            state = Game_state::player_eaten;
            return;
        }
        if (player_room->pit)
        {
            state = Game_state::player_fell;
            return;
        }
        if (player_room->bat)
        {
            out << player_dropped_in_random_room_message << std::endl;
            player_room = &rooms[random(0, num_rooms)];
            continue;
        }
        break;
    }
}

Room* Game::get_next_room_for_arrow_flight(
    const Room* previous_room, const Room* current_room, int target) const
{
    int previous_number = previous_room ? previous_room->number : 0;
    if (previous_number != target)
        for (Room* room : current_room->adjacent_rooms)
            if (room->number == target)
                return room;
    const std::array<Room*, connections_per_room>& candidate_rooms =
        current_room->adjacent_rooms;
    return candidate_rooms[random_if(
        0, connections_per_room, [candidate_rooms, previous_number](int x) {
            return candidate_rooms[x]->number != previous_number;
        })];
}

void Game::move_wumpus()
{
    out << wumpus_moves_message << std::endl;
    Room* new_wumpus_room =
        wumpus_room->adjacent_rooms[random(0, connections_per_room)];
    new_wumpus_room->wumpus = true;
    wumpus_room->wumpus = false;
    wumpus_room = new_wumpus_room;
    if (player_room->wumpus)
        state = Game_state::player_eaten;
}

const std::array<Room, num_rooms>& Game::get_rooms() const
{
    return rooms;
}

const Room* Game::get_player_room() const
{
    return player_room;
}

Game_state Game::get_game_state() const
{
    return state;
}

int Game::get_arrows() const
{
    return arrows;
}
}
