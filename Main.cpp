#include <chrono>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <string>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>


const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;
const float PADDLE_SPEED = 1.0f;
const int PADDLE_WIDTH = 10;
const int PADDLE_HEIGHT = 100;
const float BALL_SPEED = 1.0f;
const int BALL_WIDTH = 15;
const int BALL_HEIGHT = 15;


enum Buttons
{
	PaddleOneUp = 0,
	PaddleOneDown,
	PaddleTwoUp,
	PaddleTwoDown,
};


enum class CollisionType
{
	None,
	Top,
	Middle,
	Bottom,
	Left,
	Right
};


struct Contact
{
	CollisionType type;
	float penetration;
};


class Vec2
{
public:
	Vec2()
		: x(0.0f), y(0.0f)
	{}

	Vec2(float x, float y)
		: x(x), y(y)
	{}

	Vec2 operator+(Vec2 const& rhs)
	{
		return Vec2(x + rhs.x, y + rhs.y);
	}

	Vec2& operator+=(Vec2 const& rhs)
	{
		x += rhs.x;
		y += rhs.y;

		return *this;
	}

	Vec2 operator*(float rhs)
	{
		return Vec2(x * rhs, y * rhs);
	}

	float x, y;
};

class Paddle
{
public:
	Paddle(Vec2 position, Vec2 velocity)
		: position(position), velocity(velocity)
	{
		rect.x = static_cast<int>(position.x);
		rect.y = static_cast<int>(position.y);
		rect.w = PADDLE_WIDTH;
		rect.h = PADDLE_HEIGHT;
	}

	void Update(float dt)
	{
		position += velocity * dt;

		if (position.y < 0)
		{
			// Restrict to top of the screen
			position.y = 0;
		}
		else if (position.y > (WINDOW_HEIGHT - PADDLE_HEIGHT))
		{
			// Restrict to bottom of the screen
			position.y = WINDOW_HEIGHT - PADDLE_HEIGHT;
		}
	}

	void Draw(SDL_Renderer* renderer)
	{
		rect.y = static_cast<int>(position.y);

		SDL_RenderFillRect(renderer, &rect);
	}

	Vec2 position;
	Vec2 velocity;
	SDL_Rect rect{};
};


class Ball
{
public:
	Ball(Vec2 position, Vec2 velocity)
		: position(position), velocity(velocity)
	{
		rect.x = static_cast<int>(position.x);
		rect.y = static_cast<int>(position.y);
		rect.w = BALL_WIDTH;
		rect.h = BALL_HEIGHT;
	}

	void Update(float dt)
	{
		position += velocity * dt;
	}

	void Draw(SDL_Renderer* renderer)
	{
		rect.x = static_cast<int>(position.x);
		rect.y = static_cast<int>(position.y);

		SDL_RenderFillRect(renderer, &rect);
	}

	void CollideWithPaddle(Contact const& contact)
	{
		position.x += contact.penetration;
		velocity.x = -velocity.x;

		if (contact.type == CollisionType::Top)
		{
			velocity.y = -.75f * BALL_SPEED;
		}
		else if (contact.type == CollisionType::Bottom)
		{
			velocity.y = 0.75f * BALL_SPEED;
		}
	}

	void CollideWithWall(Contact const& contact)
	{
		if ((contact.type == CollisionType::Top)
		    || (contact.type == CollisionType::Bottom))
		{
			position.y += contact.penetration;
			velocity.y = -velocity.y;
		}
		else if (contact.type == CollisionType::Left)
		{
			position.x = WINDOW_WIDTH / 2.0f;
			position.y = WINDOW_HEIGHT / 2.0f;
			velocity.x = BALL_SPEED;
			velocity.y = 0.75f * BALL_SPEED;
		}
		else if (contact.type == CollisionType::Right)
		{
			position.x = WINDOW_WIDTH / 2.0f;
			position.y = WINDOW_HEIGHT / 2.0f;
			velocity.x = -BALL_SPEED;
			velocity.y = 0.75f * BALL_SPEED;
		}
	}

	Vec2 position;
	Vec2 velocity;
	SDL_Rect rect{};
};


class PlayerScore
{
public:
	PlayerScore(Vec2 position, SDL_Renderer* renderer, TTF_Font* font)
		: renderer(renderer), font(font)
	{
		surface = TTF_RenderText_Solid(font, "0", {0xFF, 0xFF, 0xFF, 0xFF});
		texture = SDL_CreateTextureFromSurface(renderer, surface);

		int width, height;
		SDL_QueryTexture(texture, nullptr, nullptr, &width, &height);

		rect.x = static_cast<int>(position.x);
		rect.y = static_cast<int>(position.y);
		rect.w = width;
		rect.h = height;
	}

	~PlayerScore()
	{
		SDL_FreeSurface(surface);
		SDL_DestroyTexture(texture);
	}

	void SetScore(int score)
	{
		SDL_FreeSurface(surface);
		SDL_DestroyTexture(texture);

		surface = TTF_RenderText_Solid(font, std::to_string(score).c_str(), {0xFF, 0xFF, 0xFF, 0xFF});
		texture = SDL_CreateTextureFromSurface(renderer, surface);

		int width, height;
		SDL_QueryTexture(texture, nullptr, nullptr, &width, &height);
		rect.w = width;
		rect.h = height;
	}

	void Draw()
	{
		SDL_RenderCopy(renderer, texture, nullptr, &rect);
	}

	SDL_Renderer* renderer;
	TTF_Font* font;
	SDL_Surface* surface{};
	SDL_Texture* texture{};
	SDL_Rect rect{};
};

class Obstacle
{
public:
    Obstacle(Vec2 position)
        : position(position)
    {
        rect.x = static_cast<int>(position.x);
        rect.y = static_cast<int>(position.y);
        rect.w = PADDLE_WIDTH;
        rect.h = PADDLE_HEIGHT;
    }

    void Draw(SDL_Renderer* renderer) const
    {
        SDL_RenderFillRect(renderer, &rect);
    }

    bool CheckCollision(const Ball& ball, Contact& contact) const
    {
        float ballLeft = ball.position.x;
        float ballRight = ball.position.x + BALL_WIDTH;
        float ballTop = ball.position.y;
        float ballBottom = ball.position.y + BALL_HEIGHT;

        float obstacleLeft = position.x;
        float obstacleRight = position.x + PADDLE_WIDTH;
        float obstacleTop = position.y;
        float obstacleBottom = position.y + PADDLE_HEIGHT;

        if (ballLeft >= obstacleRight || ballRight <= obstacleLeft || ballTop >= obstacleBottom || ballBottom <= obstacleTop)
        {
            return false;
        }

        if (ball.velocity.x < 0)
        {
            contact.penetration = obstacleRight - ballLeft;
        }
        else if (ball.velocity.x > 0)
        {
            contact.penetration = obstacleLeft - ballRight;
        }

        if (ball.velocity.y < 0)
        {
            contact.penetration = obstacleBottom - ballTop;
        }
        else if (ball.velocity.y > 0)
        {
            contact.penetration = obstacleTop - ballBottom;
        }

        return true;
    }

    Vec2 position;
    SDL_Rect rect{};
};

Contact CheckPaddleCollision(Ball const& ball, Paddle const& paddle)
{
	float ballLeft = ball.position.x;
	float ballRight = ball.position.x + BALL_WIDTH;
	float ballTop = ball.position.y;
	float ballBottom = ball.position.y + BALL_HEIGHT;

	float paddleLeft = paddle.position.x;
	float paddleRight = paddle.position.x + PADDLE_WIDTH;
	float paddleTop = paddle.position.y;
	float paddleBottom = paddle.position.y + PADDLE_HEIGHT;

	Contact contact{};

	if (ballLeft >= paddleRight)
	{
		return contact;
	}

	if (ballRight <= paddleLeft)
	{
		return contact;
	}

	if (ballTop >= paddleBottom)
	{
		return contact;
	}

	if (ballBottom <= paddleTop)
	{
		return contact;
	}

	float paddleRangeUpper = paddleBottom - (2.0f * PADDLE_HEIGHT / 3.0f);
	float paddleRangeMiddle = paddleBottom - (PADDLE_HEIGHT / 3.0f);

	if (ball.velocity.x < 0)
	{
		// Left paddle
		contact.penetration = paddleRight - ballLeft;
	}
	else if (ball.velocity.x > 0)
	{
		// Right paddle
		contact.penetration = paddleLeft - ballRight;
	}

	if ((ballBottom > paddleTop)
	    && (ballBottom < paddleRangeUpper))
	{
		contact.type = CollisionType::Top;
	}
	else if ((ballBottom > paddleRangeUpper)
	         && (ballBottom < paddleRangeMiddle))
	{
		contact.type = CollisionType::Middle;
	}
	else
	{
		contact.type = CollisionType::Bottom;
	}

	return contact;
}


Contact CheckWallCollision(Ball const& ball)
{
	float ballLeft = ball.position.x;
	float ballRight = ball.position.x + BALL_WIDTH;
	float ballTop = ball.position.y;
	float ballBottom = ball.position.y + BALL_HEIGHT;

	Contact contact{};

	if (ballLeft < 0.0f)
	{
		contact.type = CollisionType::Left;
	}
	else if (ballRight > WINDOW_WIDTH)
	{
		contact.type = CollisionType::Right;
	}
	else if (ballTop < 0.0f)
	{
		contact.type = CollisionType::Top;
		contact.penetration = -ballTop;
	}
	else if (ballBottom > WINDOW_HEIGHT)
	{
		contact.type = CollisionType::Bottom;
		contact.penetration = WINDOW_HEIGHT - ballBottom;
	}

	return contact;
}

std::vector<Obstacle> generateRandomObstacles()
{
    int numObstacles = std::rand() % 4 + 1; // Random number between 1 and 4
    std::vector<Obstacle> obstacles;

    for (int i = 0; i < numObstacles; ++i)
    {
        float x = static_cast<float>(std::rand() % (WINDOW_WIDTH - 400 - PADDLE_WIDTH) + 200);
        float y = static_cast<float>(std::rand() % (WINDOW_HEIGHT - PADDLE_HEIGHT));
        obstacles.emplace_back(Vec2(x, y));
    }

    return obstacles;
}

void DisplayWinner(SDL_Renderer* renderer, TTF_Font* font, const std::string& winner)
{
	SDL_Surface* surface = TTF_RenderText_Solid(font, winner.c_str(), {0xFF, 0xFF, 0xFF, 0xFF});
	SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);

	int width, height;
	SDL_QueryTexture(texture, nullptr, nullptr, &width, &height);
	SDL_Rect rect = {WINDOW_WIDTH / 2 - width / 2, WINDOW_HEIGHT / 2 - height / 2, width, height};

	SDL_RenderCopy(renderer, texture, nullptr, &rect);
	SDL_RenderPresent(renderer);

	SDL_FreeSurface(surface);
	SDL_DestroyTexture(texture);
}

int main()
{
	// Initialize SDL components
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	TTF_Init();
	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);

	SDL_Window* window = SDL_CreateWindow("Pong", 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);

	// Load the background image
	SDL_Surface* bgSurface = IMG_Load("bg.jpeg");
	if (!bgSurface) {
	    std::cerr << "Failed to load background image: " << IMG_GetError() << std::endl;
	    return -1;
	}

	SDL_Texture* bgTexture = SDL_CreateTextureFromSurface(renderer, bgSurface);
	SDL_FreeSurface(bgSurface);
	if (!bgTexture) {
	    std::cerr << "Failed to create texture from surface: " << SDL_GetError() << std::endl;
	    return -1;
	}

	// Initialize the font
	TTF_Font* scoreFont = TTF_OpenFont("DejaVuSansMono.ttf", 40);


	// Initialize sound effects
	Mix_Chunk* wallHitSound = Mix_LoadWAV("WallHit.wav");
	Mix_Chunk* paddleHitSound = Mix_LoadWAV("PaddleHit.wav");

	// Initialize random seed
	std::srand(static_cast<unsigned>(std::time(nullptr)));

	// Game logic
	{
		// Create the player score text fields
		PlayerScore playerOneScoreText(Vec2(WINDOW_WIDTH / 4, 20), renderer, scoreFont);

		PlayerScore playerTwoScoreText(Vec2(3 * WINDOW_WIDTH / 4, 20), renderer, scoreFont);


		// Create the ball
		Ball ball(
			Vec2(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f),
			Vec2(BALL_SPEED, 0.0f));


		// Create the paddles
		Paddle paddleOne(
			Vec2(50.0f, WINDOW_HEIGHT / 2.0f),
			Vec2(0.0f, 0.0f));

		Paddle paddleTwo(
			Vec2(WINDOW_WIDTH - 50.0f, WINDOW_HEIGHT / 2.0f),
			Vec2(0.0f, 0.0f));


		// Create the obstacles
		std::vector<Obstacle> obstacles = generateRandomObstacles();


		int playerOneScore = 0;
		int playerTwoScore = 0;

		bool running = true;
		bool buttons[4] = {};

		float dt = 0.0f;

		while (running)
		{
			auto startTime = std::chrono::high_resolution_clock::now();

			SDL_Event event;
			while (SDL_PollEvent(&event))
			{
				if (event.type == SDL_QUIT)
				{
					running = false;
				}
				else if (event.type == SDL_KEYDOWN)
				{
					if (event.key.keysym.sym == SDLK_ESCAPE)
					{
						running = false;
					}
					else if (event.key.keysym.sym == SDLK_z)
					{
						buttons[Buttons::PaddleOneUp] = true;
					}
					else if (event.key.keysym.sym == SDLK_x)
					{
						buttons[Buttons::PaddleOneDown] = true;
					}
					else if (event.key.keysym.sym == SDLK_9)
					{
						buttons[Buttons::PaddleTwoUp] = true;
					}
					else if (event.key.keysym.sym == SDLK_0)
					{
						buttons[Buttons::PaddleTwoDown] = true;
					}
				}
				else if (event.type == SDL_KEYUP)
				{
					if (event.key.keysym.sym == SDLK_z)
					{
						buttons[Buttons::PaddleOneUp] = false;
					}
					else if (event.key.keysym.sym == SDLK_x)
					{
						buttons[Buttons::PaddleOneDown] = false;
					}
					else if (event.key.keysym.sym == SDLK_9)
					{
						buttons[Buttons::PaddleTwoUp] = false;
					}
					else if (event.key.keysym.sym == SDLK_0)
					{
						buttons[Buttons::PaddleTwoDown] = false;
					}
				}
			}


			if (buttons[Buttons::PaddleOneUp])
			{
				paddleOne.velocity.y = -PADDLE_SPEED;
			}
			else if (buttons[Buttons::PaddleOneDown])
			{
				paddleOne.velocity.y = PADDLE_SPEED;
			}
			else
			{
				paddleOne.velocity.y = 0.0f;
			}

			if (buttons[Buttons::PaddleTwoUp])
			{
				paddleTwo.velocity.y = -PADDLE_SPEED;
			}
			else if (buttons[Buttons::PaddleTwoDown])
			{
				paddleTwo.velocity.y = PADDLE_SPEED;
			}
			else
			{
				paddleTwo.velocity.y = 0.0f;
			}


			// Update the paddle positions
			paddleOne.Update(dt);
			paddleTwo.Update(dt);


			// Update the ball position
			ball.Update(dt);


			// Check collisions
			if (Contact contact = CheckPaddleCollision(ball, paddleOne);
				contact.type != CollisionType::None)
			{
				ball.CollideWithPaddle(contact);

				Mix_PlayChannel(-1, paddleHitSound, 0);
			}
			else if (contact = CheckPaddleCollision(ball, paddleTwo);
				contact.type != CollisionType::None)
			{
				ball.CollideWithPaddle(contact);

				Mix_PlayChannel(-1, paddleHitSound, 0);
			}
			else if (contact = CheckWallCollision(ball);
				contact.type != CollisionType::None)
			{
				ball.CollideWithWall(contact);

				if (contact.type == CollisionType::Left)
				{
					++playerTwoScore;

					playerTwoScoreText.SetScore(playerTwoScore);
					obstacles = generateRandomObstacles();

					if (playerTwoScore >= 20)
					{
						DisplayWinner(renderer, scoreFont, "Player 2 Wins!");
						SDL_Delay(3000); // Display the winner for 3 seconds
						running = false;
					}
				}
				else if (contact.type == CollisionType::Right)
				{
					++playerOneScore;

					playerOneScoreText.SetScore(playerOneScore);
					obstacles = generateRandomObstacles();

					if (playerOneScore >= 20)
					{
						DisplayWinner(renderer, scoreFont, "Player 1 Wins!");
						SDL_Delay(10000); // Display the winner for 3 seconds
						running = false;
					}
				}
				else
				{
					Mix_PlayChannel(-1, wallHitSound, 0);
				}
			}

			// Check for collisions with obstacles
			for (const auto& obstacle : obstacles)
			{
				Contact contact;
				if (obstacle.CheckCollision(ball, contact))
				{
					ball.velocity.x = -ball.velocity.x;
					// ball.velocity.y = -ball.velocity.y;
					ball.velocity.y = -0.75f * BALL_SPEED;
				}
			}

			// Clear the window to black
			SDL_SetRenderDrawColor(renderer, 0x0, 0x0, 0x0, 0xFF);
			SDL_RenderClear(renderer);

			// Set the draw color to be white
			SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);

			// Draw the background
			SDL_RenderCopy(renderer, bgTexture, nullptr, nullptr);

			// Draw the net
			for (int y = 0; y < WINDOW_HEIGHT; ++y)
			{
				if (y % 5)
				{
					SDL_RenderDrawPoint(renderer, WINDOW_WIDTH / 2, y);
				}
			}


			// Draw the ball
			ball.Draw(renderer);

			// Draw the paddles
			paddleOne.Draw(renderer);
			paddleTwo.Draw(renderer);

			 // Draw the obstacles
			for (const auto& obstacle : obstacles)
			{
					obstacle.Draw(renderer);
			}

			// Display the scores
			playerOneScoreText.Draw();
			playerTwoScoreText.Draw();


			// Present the backbuffer
			SDL_RenderPresent(renderer);


			// Calculate frame time
			auto stopTime = std::chrono::high_resolution_clock::now();
			dt = std::chrono::duration<float, std::chrono::milliseconds::period>(stopTime - startTime).count();
		}
	}

	Mix_FreeChunk(wallHitSound);
	Mix_FreeChunk(paddleHitSound);
	SDL_DestroyTexture(bgTexture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	TTF_CloseFont(scoreFont);
	Mix_Quit();
	TTF_Quit();
	SDL_Quit();

	return 0;
}

