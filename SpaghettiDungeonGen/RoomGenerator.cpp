#include "RoomGenerator.h"
#include "Map.h"
#include "DRandom.h"

extern Map m;

RoomGenerator::RoomGenerator(int x, int y, int width, int height) : MapArray()
{
	SetSize(width, height);
	room_x = x;
	room_y = y;

	for (int j = y; j < y + height; j++)
	{
		for (int i = x; i < x + width; i++)
		{
			if (m.Get(i, j).type == TileType::Nothing)
			{
				Set(i - x, j - y, ProceduralRoomTileType::Nothing);
			}
			else
			{
				Set(i - x, j - y, ProceduralRoomTileType::Reserved);
			}
		}
	}
}

bool RoomGenerator::Generate()
{
	int place_tile_attempts = 0;
	while (true)
	{
		AddSquare();
		int count = CountTiles();

		if (count / (float)((width - 2) * (height - 2)) > 0.8f)	//width-2 and height-2 because of the nothingness border around the room
		{
			break;
		}

		if (place_tile_attempts++ == 100)
		{
			return false;
		}
	}

	for (int j = 0; j < height; j++)
	{
		for (int i = 0; i < width; i++)
		{
			if (data[j][i] == ProceduralRoomTileType::Unprocessed)	//If this room is procedural, and a neighbour is MetaNothing (or invalid), place a wall
			{
				if (CheckWallShouldBePlaced(*this, i - 1, j) ||
					CheckWallShouldBePlaced(*this, i + 1, j) ||
					CheckWallShouldBePlaced(*this, i, j - 1) ||
					CheckWallShouldBePlaced(*this, i, j + 1) ||
					CheckWallShouldBePlaced(*this, i + 1, j + 1) ||
					CheckWallShouldBePlaced(*this, i - 1, j - 1) ||
					CheckWallShouldBePlaced(*this, i + 1, j - 1) ||
					CheckWallShouldBePlaced(*this, i - 1, j + 1))
				{
					if (Get(i, j) != ProceduralRoomTileType::Reserved)
					{
						Set(i, j, ProceduralRoomTileType::Wall);
					}
				}
			}
		}
	}

	//For double thickness walls
	/*MapArray map_copy = *this;
	for (int j = 0; j < height; j++)
	{
		for (int i = 0; i < width; i++)
		{
			if (data[j][i] == ProceduralRoomTileType::Unprocessed)	//If this room is procedural, and a neighbour is a wall, place a wall
			{
				if (CanDilateWallIntoRoom(map_copy, i - 1, j) ||
					CanDilateWallIntoRoom(map_copy, i + 1, j) ||
					CanDilateWallIntoRoom(map_copy, i, j - 1) ||
					CanDilateWallIntoRoom(map_copy, i, j + 1) ||
					CanDilateWallIntoRoom(map_copy, i + 1, j + 1) ||
					CanDilateWallIntoRoom(map_copy, i - 1, j - 1) ||
					CanDilateWallIntoRoom(map_copy, i + 1, j - 1) ||
					CanDilateWallIntoRoom(map_copy, i - 1, j + 1))
				{
					if (map_copy.Get(i, j) != ProceduralRoomTileType::Reserved)
					{
						Set(i, j, ProceduralRoomTileType::Wall);
					}
				}
			}
		}
	}*/

	for (int j = 0; j < height; j++)
	{
		for (int i = 0; i < width; i++)
		{
			ProceduralRoomTileType type = Get(i, j);
			if (type == ProceduralRoomTileType::Unprocessed)
			{
				tile_wall_data.tiles.push_back({ i, j });
			}
			else if (type == ProceduralRoomTileType::Wall)
			{
				tile_wall_data.walls.push_back({ i, j });
			}
		}
	}

	//todo: ensure it's all connected

	//PlaceDoors();

	PlaceSingleDoors();


	//todo: ensure doors are ok

	m.AddRoom(room_x, room_y, possible_doors, std::move(std::make_unique<TileWallData>(tile_wall_data))).room_type = RoomType::Procedural;

	return true;
}

bool RoomGenerator::CheckWallShouldBePlaced(const MapArray<ProceduralRoomTileType>& map_array, int x, int y)	//Place wall if the neighbour (x, y in this function) is either invalid, an area outside the room so far (Nothing), or reserved
{
	if (map_array.IsValid(x, y))
	{
		return map_array.Get(x, y) == ProceduralRoomTileType::Nothing || map_array.Get(x, y) == ProceduralRoomTileType::Reserved;
	}
	else
	{
		return true;
	}
}

bool RoomGenerator::CanDilateWallIntoRoom(const MapArray<ProceduralRoomTileType>& map_array, int x, int y)	//Place wall if the neighbour (x, y in this function) is a wall.
{
	if (map_array.IsValid(x, y))
	{
		return map_array.Get(x, y) == ProceduralRoomTileType::Wall;
	}
	else
	{
		return false;
	}
}

void RoomGenerator::AddSquare()
{
	//Squares can start at 1 rather than 0 so that there's often a border of nothingness around the room
	int start_x = DRandom::instance->GetInt(1, width - 1);
	int start_y = DRandom::instance->GetInt(1, height - 1);
	int end_x = DRandom::instance->GetInt(start_x, width - 1);
	int end_y = DRandom::instance->GetInt(start_y, height - 1);

	for (int j = start_y; j < end_y; j++)
	{
		for (int i = start_x; i < end_x; i++)
		{
			if (Get(i, j) != ProceduralRoomTileType::Unprocessed && Get(i, j) != ProceduralRoomTileType::Reserved)	//If tile hasn't been placed here already
			{
				Set(i, j, ProceduralRoomTileType::Unprocessed);
			}
		}
	}
}

int RoomGenerator::CountTiles()
{
	int count = 0;
	for (int j = 0; j < height; j++)
	{
		for (int i = 0; i < width; i++)
		{
			if (Get(i, j) == ProceduralRoomTileType::Unprocessed)
			{
				count++;
			}
		}
	}
	return count;
}

//Pick a random row. Then starting at column 0, move inwards until we find a wall. The next tile should be a wall, as well as the tiles left and right
void RoomGenerator::PlaceDoors()
{
	int x;
	int y;
	int x_direction;
	int y_direction;
	int movement_amount;

	for (int i = 0; i < 50; i++)	//50 attempts to place doors
	{
		int rng = DRandom::instance->GetInt(0, 3);
		switch (rng)
		{
		case 0:	//Random row. Column moving left to right.
			y_direction = 0;
			y = DRandom::instance->GetInt(2, height - 2);
			x_direction = 1;
			x = 0;
			movement_amount = width - 2;
			break;
		case 1: //Random row. Column moving right to left.
			y_direction = 0;
			y = DRandom::instance->GetInt(2, height - 2);
			x_direction = -1;
			x = width - 1;
			movement_amount = width - 2;
			break;
		case 2:	//Random column. Row moving top to bottom.
			y_direction = 1;
			y = 0;
			x_direction = 0;
			x = DRandom::instance->GetInt(2, width - 2);
			movement_amount = height - 2;
			break;
		case 3: //Random column. Row moving bottom to top.
			y_direction = -1;
			y = height - 1;
			x_direction = 0;
			x = DRandom::instance->GetInt(2, width - 2);
			movement_amount = height - 2;
			break;
		}

		for (int movement = 0; movement < movement_amount; movement++)
		{
			if (Get(x, y) == ProceduralRoomTileType::Wall)
			{
				if (DoorCanBePlacedAtCaller(x + y_direction, y + x_direction) &&	//x, y + 1
					DoorCanBePlacedAtCaller(x - y_direction, y - x_direction) &&	//x, y - 1
					DoorCanBePlacedAtCaller(x + x_direction, y + y_direction) &&	//x + 1, y
					DoorCanBePlacedAtCaller(x + x_direction + y_direction, y + x_direction + y_direction) &&	//x + 1, y + 1
					DoorCanBePlacedAtCaller(x + x_direction - y_direction, y - x_direction + y_direction) &&	//x + 1, y - 1
					Get(x + 2 * x_direction, y + 2 * y_direction) == ProceduralRoomTileType::Unprocessed && //Ensure that x + 2, y is a tile within the room
					TileOutsideOfRoom(x - x_direction, y - y_direction))	//x - 1, y is outside of the room
				{
					if (x_direction == -1 || y_direction == -1)
					{
						possible_doors.push_back(PossibleDoor(x + x_direction, y + y_direction, x, y, tile_wall_data));	//The first two arguments need to be earlier in the array
					}
					else
					{
						possible_doors.push_back(PossibleDoor(x, y, x + x_direction, y + y_direction, tile_wall_data));
					}
					data[y][x] = ProceduralRoomTileType::PossibleConnection;
					data[y + y_direction][x + x_direction] = ProceduralRoomTileType::PossibleConnection;
					break;
				}
			}

			x += x_direction;
			y += y_direction;
		}

		if (possible_doors.size() == 5)	//5 doors max
		{
			break;
		}
	}

	//todo: check we have minimum number of doors
}

void RoomGenerator::PlaceSingleDoors()
{
	int x;
	int y;
	int x_direction;
	int y_direction;
	int movement_amount;

	for (int i = 0; i < 50; i++)	//50 attempts to place doors
	{
		int rng = DRandom::instance->GetInt(0, 3);
		switch (rng)
		{
		case 0:	//Random row. Column moving left to right.
			y_direction = 0;
			y = DRandom::instance->GetInt(2, height - 2);
			x_direction = 1;
			x = 0;
			movement_amount = width - 2;
			break;
		case 1: //Random row. Column moving right to left.
			y_direction = 0;
			y = DRandom::instance->GetInt(2, height - 2);
			x_direction = -1;
			x = width - 1;
			movement_amount = width - 2;
			break;
		case 2:	//Random column. Row moving top to bottom.
			y_direction = 1;
			y = 0;
			x_direction = 0;
			x = DRandom::instance->GetInt(2, width - 2);
			movement_amount = height - 2;
			break;
		case 3: //Random column. Row moving bottom to top.
			y_direction = -1;
			y = height - 1;
			x_direction = 0;
			x = DRandom::instance->GetInt(2, width - 2);
			movement_amount = height - 2;
			break;
		}

		for (int movement = 0; movement < movement_amount; movement++)
		{
			if (Get(x, y) == ProceduralRoomTileType::Wall)
			{
				if (Get(x + x_direction, y + y_direction) == ProceduralRoomTileType::Unprocessed && //Ensure that x + 1, y is a tile within the room
					TileOutsideOfRoom(x - x_direction, y - y_direction))	//x - 1, y is outside of the room
				{
					possible_doors.push_back(PossibleDoor(x, y, -1, -1, tile_wall_data));	//The first two arguments need to be earlier in the array
					data[y][x] = ProceduralRoomTileType::PossibleConnection;
					break;
				}
			}

			x += x_direction;
			y += y_direction;
		}

		if (possible_doors.size() == 5)	//5 doors max
		{
			break;
		}
	}

	//todo: check we have minimum number of doors
}

bool RoomGenerator::DoorCanBePlacedAtCaller(int i, int j)
{
	return Get(i, j) == ProceduralRoomTileType::Wall;
}

bool RoomGenerator::TileOutsideOfRoom(int i, int j)
{
	if (IsValid(i, j))
	{
		return Get(i, j) == ProceduralRoomTileType::Nothing;
	}
	else
	{
		return false;	//Invalid tiles aren't outside of the room. This is to stop doors being placed next to walls that belong to the rest of the map
	}
}