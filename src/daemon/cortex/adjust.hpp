#pragma once

#include "def.hpp"
#include "logger.hpp"
#include "types.hpp"

#include <functional>

/**
 * @brief Speed and steering adjustment
 * @author Luu Ngoc Tran
 * @author Andrej Utz
 */
struct Adjust
{
	/**
	 * @brief Horizontal position
	 */
	struct Line
	{
		u16 max,  ///< Number of neighbors in line
		    pos;  ///< Own position in line from left
	};

	/**
	 * @brief Updateable value storing previos ones
	 */
	struct Value
	{
		/**
		 * @brief Implicit usage as curr in i32 contextes
		 */
		operator i32() { return curr; };
	private:
		friend struct Adjust;
		i32 prev = 0, curr = 0, target = 0;
		inline i32 update(i32 new_val) { auto p = prev; prev = curr; curr = new_val; return p; }
		inline i32 diff() const { return prev - curr; }
	};

	Value speed, steer, gap, cam;
	const Line line;

	/**
	 * @param Line definition for steering
	 */
	Adjust(Line&& line);

	/**
	 * @param spd User speed input
	 */
	void speed_update(i32 spd);
	/**
	 * @param deg User steer input
	 */
	void steer_update(i32 deg);
	/**
	 * @param mm Gap distance from side
	 */
	void gap_update(i32 mm);
	/**
	 * @param diff Camera offset from side
	 */
	void cam_update(f32 diff);

	/**
	 * @brief For cars without track equipment
	 * @param mm Camera offset other side
	 */
	void gap_inner(i32 mm);

	/**
	 * @brief Callback to drive controls
	 */
	std::function<void(i32 speed)> drive;
	/**
	 * @brief Callback to steer controls
	 */
	std::function<void(i32 degree)> steering;

private:
	void adjust_speed(f32 spd);
	void adjust_steer(f32 deg);

	loggr logger;
};

