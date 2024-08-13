typedef enum SpriteID {
	SPRITE_NIL,
	SPRITE_PLAYER,
	SPRITE_CIRCLE,
	SPRITE_SKELETON,
	SPRITE_MAX
} SpriteID;

#define SPRITE_SIZE 16.0
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
	float64 seconds_count 	= 0.0;
	int frame_count 		= 0;

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
	}

	for(int i = 0; i < 10; i++) {
		Entity* entity = entity_create();
		setup_skeleton(entity);
		entity->pos = v2(get_random_float32_in_range(-500, 500), get_random_float32_in_range(-500, 500));
	}

	while (!window.should_close) {
		reset_temporary_storage();
		os_update(); 
		gfx_update();

		// :view
		float zoom 				= 3;
		draw_frame.projection 	= m4_make_orthographic_projection(window.width * -0.5, window.width * 0.5, window.height * -0.5, window.height * 0.5, -1, 10);
		draw_frame.view 		= m4_scalar(1.0);
		draw_frame.view 		= m4_mul(draw_frame.view, m4_make_translation(v3(player_entity->pos.x, player_entity->pos.y, 0)));
		draw_frame.view 		= m4_mul(draw_frame.view, m4_make_scale(v3(1.0/zoom, 1.0/zoom, 1.0)));

		// :time
		float64 now 	= os_get_current_time_in_seconds();
		float64 delta_t	= now - last_time;
		last_time 		= now;

		// :rendering
		for(int i = 0; i < MAX_ENTITY_PER_WORLD; i++) {
			Entity* entity = &world->entities[i];
			if(entity->is_valid) {
				Sprite* sprite		= get_sprite(entity->sprite_id);
				Matrix4 xform		= m4_scalar(1.0);
				xform				= m4_translate(xform, v3(entity->pos.x, entity->pos.y, 0));
				xform				= m4_translate(xform, v3(sprite->size.x * -0.5, 0.0, 0));
				draw_image_xform(sprite->image, xform, sprite->size, COLOR_WHITE);
			}
		}

		// :input
		if(is_key_just_pressed(KEY_ESCAPE)) {
			window.should_close = true;
		}

		Vector2 input_axis = v2(0, 0);
		if (is_key_down('Q')) {
			input_axis.x -= 1.0;
		}
		if (is_key_down('D')) {
			input_axis.x += 1.0;
		}
		if (is_key_down('S')) {
			input_axis.y -= 1.0;
		}
		if (is_key_down('Z')) {
			input_axis.y += 1.0;
		}
		input_axis = v2_normalize(input_axis);

		player_entity->pos = v2_add(player_entity->pos, v2_mulf(input_axis, 128 * delta_t));

		// :fps
		seconds_count 	+= delta_t;
		frame_count 	+= 1;
		if (seconds_count > 1.0) {
			log("fps: %i", frame_count);
			seconds_count 	= 0.0;
			frame_count 	= 0;
		}
	}

	return 0;
}