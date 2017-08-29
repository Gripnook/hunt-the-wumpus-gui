#define _ENABLE_ATOMIC_ALIGNMENT_FIX

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Text.h"

#include <array>
#include <string>
#include <sstream>
#include <memory>
#include <atomic>

#include "game.h"

using namespace ci;
using namespace ci::app;

using namespace wumpus;

struct Action
{
    enum class Action_type
    {
        none,
        move,
        shoot,
        draw,
        quit,
        help
    } type{Action_type::none};
    int target;

    Action() = default;

    Action(Action_type type) : type{type}
    {
    }

    Action(Action_type type, int target) : type{type}, target{target}
    {
    }
};

class HuntTheWumpusApp : public App
{
public:
    static std::string titleScreenText();

    void setup() override;
    void keyUp(KeyEvent event) override;
    void mouseUp(MouseEvent event) override;
    void update() override;
    void draw() override;

private:
    std::atomic<bool> isEventTriggered{false};
    std::atomic<bool> isTitleScreen{true};
    std::atomic<bool> isGameOver{false};
    std::atomic<bool> isShootEnabled{false};
    std::atomic<bool> isDrawEnabled{false};
    std::atomic<Action> nextAction;
    std::atomic<int> arrows;

    std::unique_ptr<Game> game;
    float consoleHeight;

    std::stringstream buffer;
    std::string outputText;

    std::array<bool, num_rooms> markedRooms{false};

    void initialize();

    void updateAction();
    void updateActionTaken();
    void updateOutputText();

    void drawTitleScreen();
    void drawBackground();
    void drawHUD();
    void drawCave();
    void drawCaveRooms();
    void drawCaveConnections();
    void drawConsole();

    vec2 getCenter(int roomNumber, vec2 caveSize) const;
    float getRadius(vec2 caveSize) const;
    bool isOnCircle(vec2 position, vec2 center, float radius) const;
};

std::string HuntTheWumpusApp::titleScreenText()
{
    std::stringstream ss;
    ss << Game::game_info();
    ss << "During each turn you must make a move. The possible moves are:"
       << std::endl
       << "    \"m #\": Move to an adjacent room." << std::endl
       << "    \"s #\": Shoot an arrow through the room specified. The range"
       << std::endl
       << "        of an arrow is " << arrow_range
       << " rooms, and a path will be chosen at random." << std::endl
       << "        You have " << num_arrows
       << " arrows at the start of the game." << std::endl
       << "    \"d\": Enter draw mode to mark rooms as dangerous." << std::endl
       << "    \"q\": Quit the game and flee the cave." << std::endl
       << "    \"h\": Pause the game and return to the title screen."
       << std::endl
       << "Good luck!" << std::endl;
    return ss.str();
}

void HuntTheWumpusApp::setup()
{
    game = std::make_unique<Game>(buffer);
    consoleHeight = 120.0f;
    initialize();
}

void HuntTheWumpusApp::initialize()
{
    game->init_hunt();
    game->inform_player_of_hazards();
    arrows = game->get_arrows();
    updateOutputText();
    isShootEnabled = false;
    isDrawEnabled = false;
    markedRooms = {false};
    nextAction = Action(Action::Action_type::none);
}

void HuntTheWumpusApp::keyUp(KeyEvent event)
{
    isEventTriggered = true;

    auto key = event.getChar();
    switch (key)
    {
    case 'm':
        isShootEnabled = false;
        isDrawEnabled = false;
        break;
    case 's':
        isShootEnabled = true;
        isDrawEnabled = false;
        break;
    case 'd':
        isDrawEnabled = !isDrawEnabled;
        break;
    case 'q':
        nextAction = Action(Action::Action_type::quit);
        break;
    case 'h':
        nextAction = Action(Action::Action_type::help);
        break;
    default:
        // Do nothing.
        break;
    }
}

void HuntTheWumpusApp::mouseUp(MouseEvent event)
{
    isEventTriggered = true;

    auto windowSize = gl::getViewport().second;
    vec2 caveSize(windowSize.x, windowSize.y - consoleHeight);

    // Which room did the user click on?
    for (int i = 0; i < num_rooms; ++i)
    {
        auto center = getCenter(i, caveSize);
        auto radius = getRadius(caveSize);

        if (isOnCircle(event.getPos(), center, radius))
        {
            if (isDrawEnabled)
                nextAction = Action(Action::Action_type::draw, i);
            else if (isShootEnabled)
                nextAction = Action(Action::Action_type::shoot, i);
            else
                nextAction = Action(Action::Action_type::move, i);

            break;
        }
    }
}

void HuntTheWumpusApp::update()
{
    if (isEventTriggered)
    {
        isEventTriggered = false;

        if (isTitleScreen)
        {
            isTitleScreen = false;
            nextAction = Action(Action::Action_type::none);
        }
        else if (isGameOver)
        {
            isTitleScreen = true;
            isGameOver = false;
            initialize();
        }
        else
        {
            updateAction();
        }
    }
}

void HuntTheWumpusApp::updateAction()
{
    Action action = nextAction;
    nextAction = Action(Action::Action_type::none);
    switch (action.type)
    {
    case Action::Action_type::none:
    {
        break;
    }
    case Action::Action_type::move:
    {
        auto target = game->get_rooms()[action.target].number;
        if (game->can_move(target))
            game->move(target);
        updateActionTaken();
        break;
    }
    case Action::Action_type::shoot:
    {
        auto target = std::array<int, connections_per_room>{
            game->get_rooms()[action.target].number, -1, -1};
        if (game->can_shoot(target))
        {
            game->shoot(target);
            isShootEnabled = false; // Allow one shot before switching back.
        }
        updateActionTaken();
        break;
    }
    case Action::Action_type::draw:
    {
        markedRooms[action.target] = !markedRooms[action.target];
        break;
    }
    case Action::Action_type::quit:
    {
        game->quit();
        updateActionTaken();
        break;
    }
    case Action::Action_type::help:
    {
        isTitleScreen = true;
        break;
    }
    default:
    {
        throw std::logic_error("Invalid player action");
    }
    }
}

void HuntTheWumpusApp::updateActionTaken()
{
    if (game->is_hunt_over())
    {
        game->end_hunt();
        isGameOver = true;
    }
    else
    {
        game->inform_player_of_hazards();
    }
    arrows = game->get_arrows();
    updateOutputText();
}

void HuntTheWumpusApp::updateOutputText()
{
    outputText = buffer.str();
    buffer = std::stringstream{};
}

void HuntTheWumpusApp::draw()
{
    gl::clear();
    if (isTitleScreen)
    {
        drawTitleScreen();
    }
    else
    {
        drawBackground();
        drawHUD();
        drawCave();
        drawConsole();
    }
}

void HuntTheWumpusApp::drawTitleScreen()
{
    gl::drawString(
        titleScreenText(),
        vec2(0.0f, 0.0f),
        Color(0.0f, 1.0f, 0.0f),
        Font("Consolas", 20));
}

void HuntTheWumpusApp::drawBackground()
{
    if (isGameOver)
    {
        if (game->get_game_state() == Game_state::wumpus_dead)
        {
            gl::clear(Color(0.0f, 0.25f, 0.0f));
        }
        else
        {
            gl::clear(Color(0.25f, 0.0f, 0.0f));
        }
    }
    else if (isDrawEnabled)
    {
        gl::clear(Color(0.2f, 0.2f, 0.2f));
    }
    else if (isShootEnabled)
    {
        gl::clear(Color(0.0f, 0.0f, 0.25f));
    }
}

void HuntTheWumpusApp::drawHUD()
{
    std::string value = isShootEnabled ? "SHOOT" : "MOVE";
    value = isDrawEnabled ? "DRAW" : value;

    value += "\n";
    value += "ARROWS: " + std::to_string(arrows);
    gl::drawString(
        value, vec2(0.0f, 0.0f), Color(0.0f, 1.0f, 0.0f), Font("Consolas", 32));
}

void HuntTheWumpusApp::drawCave()
{
    drawCaveConnections();
    drawCaveRooms();
}

void HuntTheWumpusApp::drawCaveRooms()
{
    auto windowSize = gl::getViewport().second;
    vec2 caveSize(windowSize.x, windowSize.y - consoleHeight);

    auto playerRoom = game->get_player_room();
    auto rooms = game->get_rooms();
    for (int i = 0; i < num_rooms; ++i)
    {
        auto center = getCenter(i, caveSize);
        auto radius = getRadius(caveSize);

        if (rooms[i].number == playerRoom->number)
            gl::color(Color(0.80f, 1.0f, 0.80f));
        else
            gl::color(Color(0.60f, 0.60f, 0.60f));

        gl::drawSolidCircle(center, radius);

        gl::drawString(
            std::to_string(rooms[i].number),
            center,
            Color(0.0f, 0.0f, 0.0f),
            Font("Consolas", radius));

        if (markedRooms[i])
        {
            // Draw an X over the room.
            gl::color(Color(0.0f, 0.0f, 0.0f));
            gl::lineWidth(std::max(radius / 10.0f, 1.0f));

            vec2 v1(radius * 0.707f, radius * 0.707f);
            gl::drawLine(center - v1, center + v1);
            vec2 v2(radius * 0.707f, -radius * 0.707f);
            gl::drawLine(center - v2, center + v2);

            gl::lineWidth(1.0f);
        }
    }
}

void HuntTheWumpusApp::drawCaveConnections()
{
    auto windowSize = gl::getViewport().second;
    vec2 caveSize(windowSize.x, windowSize.y - consoleHeight);

    auto playerRoom = game->get_player_room();
    auto rooms = game->get_rooms();
    for (int i = 0; i < num_rooms; ++i)
    {
        auto center = getCenter(i, caveSize);

        for (int j = 0; j < connections_per_room; ++j)
        {
            gl::color(Color(0.60f, 0.60f, 0.60f));
            gl::drawLine(center, getCenter(room_connections[i][j], caveSize));
        }
    }
}

void HuntTheWumpusApp::drawConsole()
{
    vec2 offset(0.0f, gl::getViewport().second.y - consoleHeight);
    gl::drawString(
        outputText, offset, Color(0.0f, 1.0f, 0.0f), Font("Consolas", 32));
}

vec2 HuntTheWumpusApp::getCenter(int roomNumber, vec2 caveSize) const
{
    auto roomRadius = 3.0f * getRadius(caveSize);
    if (roomNumber < 5)
    {
        roomNumber *= 2;
    }
    else if (roomNumber < 15)
    {
        roomNumber -= 5;
        roomRadius *= 2.0f;
    }
    else
    {
        roomNumber -= 15;
        roomNumber *= 2;
        roomNumber += 1;
        roomRadius *= 3.0f;
    }
    auto x = roomRadius * std::sinf(2.0f * (float) M_PI * roomNumber / 10.0f);
    auto y = roomRadius * std::cosf(2.0f * (float) M_PI * roomNumber / 10.0f);
    return vec2(x, y) + caveSize / 2.0f;
}

float HuntTheWumpusApp::getRadius(vec2 caveSize) const
{
    return std::min(caveSize.x, caveSize.y) / 22.5f;
}

bool HuntTheWumpusApp::isOnCircle(vec2 position, vec2 center, float radius) const
{
    auto delta = position - center;
    return delta.x * delta.x + delta.y * delta.y <= radius * radius;
}

CINDER_APP(HuntTheWumpusApp, RendererGl)
