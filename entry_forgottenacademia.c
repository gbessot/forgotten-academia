bool almost_equal(float a, float b, float epsilon) {
	return fabs(a - b) <= epsilon;
}

bool animate_f32_to_target(float* value, float target, float delta_t, float rate) {
	*value += (target - *value) * (1.0f - pow(2.0f, -rate * delta_t));
	if(almost_equal(*value, target, 0.001f)) {
		*value = target;
		return true;
	}
	return false;
}

bool animate_v2_to_target(Vector2* value, Vector2 target, float delta_t, float rate) {
	bool xIsReached = animate_f32_to_target(&(value->x), target.x, delta_t, rate);
	bool yIsReached = animate_f32_to_target(&(value->y), target.y, delta_t, rate);
	return xIsReached && yIsReached;
}

Vector2 get_mouse_pos_in_world() {
	float mouse_x = input_frame.mouse_x;
	float mouse_y = input_frame.mouse_y;

	float ndcX = (input_frame.mouse_x / window.width) * 2 - 1;
	float ndcY = (input_frame.mouse_y / window.height) * 2 - 1;

	Vector4 world_pos 	= v4(ndcX, ndcY, 0, 1);
	world_pos			= m4_transform(m4_inverse(draw_frame.projection), world_pos);
	world_pos			= m4_transform(draw_frame.view, world_pos);

	return v2(world_pos.x, world_pos.y);
}

const float TILE_SIZE = 16.0f;
int world_pos_to_tile_pos(float world_pos) {
	return floor(world_pos / TILE_SIZE);
}

float tile_pos_to_world_pos(float tile_pos) {
	return tile_pos * TILE_SIZE;
}

Vector2 round_v2_to_tile(Vector2 world_pos) {
	world_pos.x = tile_pos_to_world_pos(world_pos_to_tile_pos(world_pos.x));
	world_pos.y = tile_pos_to_world_pos(world_pos_to_tile_pos(world_pos.y));
	return world_pos;
}


typedef enum SpriteID {
	SPRITE_NIL,
	SPRITE_PLAYER,
	SPRITE_CIRCLE,
	SPRITE_SKELETON,
	SPRITE_MAX
} SpriteID;

const float SPRITE_SIZE = 16.0f;
typedef struct Sprite {
	Gfx_Image* 	image;
	Vector2 	size;
} Sprite;

Sprite sprites[SPRITE_MAX];
Sprite* get_sprite(SpriteID id) {
	if(id >= 0 && id < SPRITE_MAX) {
		return &sprites[id];
	}
	return &sprites[0];
}

typedef enum EntityType {
	ENTITY_PLAYER,
	ENTITY_CIRCLE,
	ENTITY_SKELETON
} EntityType;

typedef struct Entity {
	bool 		is_valid;
	EntityType 	type;
	SpriteID 	sprite_id;
	Vector2 	pos;
} Entity;

#define MAX_ENTITY_PER_WORLD 1024
typedef struct World {
	Entity entities[MAX_ENTITY_PER_WORLD];
} World;
World* world = NULL;

Entity* entity_create() {
	Entity* found_entity = NULL;
	for(int i = 0; i < MAX_ENTITY_PER_WORLD; i++) {
		Entity* existing_entity = &world->entities[i];
		if(!existing_entity->is_valid) {
			found_entity = existing_entity;
			break;
		}
	}
	assert(found_entity, "Max entity per world exceeded.");
	found_entity->is_valid = true;
	return found_entity;
}

void entity_destroy(Entity* entity) {
	memset(entity, 0, sizeof(Entity));
}

void setup_player(Entity* entity) {
	entity->type 		= ENTITY_PLAYER;
	entity->sprite_id 	= SPRITE_PLAYER;
}

void setup_circle(Entity* entity) {
	entity->type 		= ENTITY_CIRCLE;
	entity->sprite_id 	= SPRITE_CIRCLE;
}

void setup_skeleton(Entity* entity) {
	entity->type 		= ENTITY_SKELETON;
	entity->sprite_id 	= SPRITE_SKELETON;
}

int entry(int argc, char **argv) {
	window.title 			= STR("Forgotten Academia");
	window.scaled_width 	= 1280;
	window.scaled_height 	= 720; 
	window.x 				= 200;
	window.y 				= 200;
	window.clear_color 		= hex_to_rgba(0x4f4263ff);

	float64 last_time 		= os_get_current_time_in_seconds();
	float64 seconds_count 	= 0.0f;
	int frame_count 		= 0;

	float zoom 				= 3.0f;
	Vector2 camera_pos		= v2(0.0f, 0.0f);

	sprites[SPRITE_PLAYER] 		= (Sprite){ .image=load_image_from_disk(STR("player.png"), 		get_heap_allocator()), .size=v2(SPRITE_SIZE, SPRITE_SIZE) };
	sprites[SPRITE_CIRCLE] 		= (Sprite){ .image=load_image_from_disk(STR("circle.png"), 		get_heap_allocator()), .size=v2(SPRITE_SIZE, SPRITE_SIZE) };
	sprites[SPRITE_SKELETON]	= (Sprite){ .image=load_image_from_disk(STR("skeleton.png"), 	get_heap_allocator()), .size=v2(SPRITE_SIZE, SPRITE_SIZE) };

	world = alloc(get_heap_allocator(), sizeof(World));
	memset(world, 0, sizeof(World));

	Entity* player_entity = entity_create();
	setup_player(player_entity);

	for(int i = 0; i < 10; i++) {
		Entity* entity = entity_create();
		setup_circle(entity);
		entity->pos = v2(get_random_float32_in_range(-500, 500), get_random_float32_in_range(-500, 500));
		entity->pos = round_v2_to_tile(entity->pos);
	}

	for(int i = 0; i < 10; i++) {
		Entity* entity = entity_create();
		setup_skeleton(entity);
		entity->pos = v2(get_random_float32_in_range(-500, 500), get_random_float32_in_range(-500, 500));
		entity->pos = round_v2_to_tile(entity->pos);
	}

	while (!window.should_close) {
		reset_temporary_storage();
		os_update(); 
		gfx_update();
		
		// :time
		float64 now 	= os_get_current_time_in_seconds();
		float64 delta_t	= now - last_time;
		last_time 		= now;

		// :view
		animate_v2_to_target(&camera_pos, player_entity->pos, delta_t, 30.0f);
		draw_frame.projection 	= m4_make_orthographic_projection(window.width * -0.5f, window.width * 0.5f, window.height * -0.5f, window.height * 0.5f, -1, 10);
		draw_frame.view 		= m4_scalar(1.0);
		draw_frame.view 		= m4_mul(draw_frame.view, m4_make_translation(v3(camera_pos.x, camera_pos.y, 0)));
		draw_frame.view 		= m4_mul(draw_frame.view, m4_make_scale(v3(1.0f/zoom, 1.0f/zoom, 1.0f)));

		// :rendering
		int tile_radius = 40;
		int player_tile_x = world_pos_to_tile_pos(player_entity->pos.x);
		int player_tile_y = world_pos_to_tile_pos(player_entity->pos.y);
		for(int y = player_tile_y - tile_radius; y < player_tile_y + tile_radius; y++) {
			for(int x = player_tile_x - tile_radius; x < player_tile_x + tile_radius; x++) {
				if((y + x) % 2 == 0) {
					draw_rect(v2(x * TILE_SIZE, y * TILE_SIZE), v2(TILE_SIZE, TILE_SIZE), hex_to_rgba(0x00000011));
				}
			}
		}
		
		for(int i = 0; i < MAX_ENTITY_PER_WORLD; i++) {
			Entity* entity = &world->entities[i];
			if(entity->is_valid) {
				Sprite* sprite		= get_sprite(entity->sprite_id);
				Matrix4 xform		= m4_scalar(1.0);
				xform				= m4_translate(xform, v3(entity->pos.x, entity->pos.y, 0));
				draw_image_xform(sprite->image, xform, sprite->size, COLOR_WHITE);
			}
		}

		// :input
		if(is_key_just_pressed(KEY_ESCAPE)) {
			window.should_close = true;
		}

		Vector2 input_axis = v2(0, 0);
		if (is_key_down('Q')) {
			input_axis.x -= 1.0f;
		}
		if (is_key_down('D')) {
			input_axis.x += 1.0f;
		}
		if (is_key_down('S')) {
			input_axis.y -= 1.0f;
		}
		if (is_key_down('Z')) {
			input_axis.y += 1.0f;
		}
		input_axis = v2_normalize(input_axis);
		player_entity->pos = v2_add(player_entity->pos, v2_mulf(input_axis, 128 * delta_t));

		Vector2 mouse_pos = get_mouse_pos_in_world();
		Vector2 mouse_tile = round_v2_to_tile(mouse_pos);
		draw_rect(mouse_tile, v2(TILE_SIZE, TILE_SIZE), hex_to_rgba(0xffffff11));

		// :fps
		seconds_count 	+= delta_t;
		frame_count 	+= 1;
		if (seconds_count > 1.0f) {
			log("fps: %i", frame_count);
			seconds_count 	= 0.0f;
			frame_count 	= 0;
		}
	}

	return 0;
}