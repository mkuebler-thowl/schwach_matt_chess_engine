#include "ChessEngine.hpp"
#include "ChessValidation.hpp"
#include "ChessEvaluation.hpp"
#include <algorithm>
#include <chrono>

namespace owl
{
	using clock = std::chrono::steady_clock;
	using time_point = std::chrono::steady_clock::time_point;
	using milliseconds = std::chrono::milliseconds;
	using microseconds = std::chrono::microseconds;

	inline time_point START_CLOCK()
	{
		return clock::now();
	}

	inline INT32 CALCULATE_MILLISECONDS(time_point start)
	{
		return std::chrono::duration_cast<milliseconds>(clock::now() - start).count();
	}

	inline auto CALCULATE_MICROSECONDS(time_point start)
	{
		return std::chrono::duration_cast<microseconds>(clock::now() - start).count();
	}

	ChessEngine::ChessEngine()
	{

	}
	ChessEngine::~ChessEngine()
	{
	}

	Move ChessEngine::searchMove(Position& position, short player, UINT16 depth, UCHAR parameterFlags, BOOL random)
	{
		// Die Position des generischen Zugs zur RepitionMap hinzufügen
		m_repitionMap.addPosition(position);

		BOOL sort = parameterFlags & FT_SORT;
		BOOL killer = parameterFlags & FT_KILLER;

		MinMaxResult result;

		// MinMax oder MinMax+Sort
		if (parameterFlags == FT_NULL || parameterFlags == FT_SORT)
		{
			result = minMax(position, player, depth);
		}
		// AlphaBeta oder AlphaBeta+Sort
		else if (parameterFlags & FT_ALPHA_BETA)
		{ 
			result = alphaBeta(position, player, depth, -INF, INF, sort, killer);
		}

		//// Nested oder Nested+Sort
		//else if (parameterFlags == FT_NESTED || parameterFlags == (FT_NESTED | FT_SORT))
		//{
		//	result = nested(position, player, depth, sort);
		//}
		//// Nested+AlphaBeta oder Nested+AlphaBeta+Sort
		//else if (parameterFlags == (FT_NESTED | FT_ALPHA_BETA) || parameterFlags == (FT_NESTED | FT_ALPHA_BETA | FT_SORT))
		//{
		//	result = nestedAlphaBeta(position, player, depth, -INF, INF, sort);
		//}

		//std::cout << (parameter_flags == (FT_ALPHA_BETA | FT_NESTED) || parameter_flags == (FT_ALPHA_BETA | FT_NESTED | FT_SORT)) << std::endl;
		//std::cout << (parameter_flags == FT_NESTED || parameter_flags == (FT_NESTED | FT_SORT)) << std::endl;

		m_repitionMap.addPosition(ChessValidation::applyMove(position, result.best));

		return result.best;
	}

	MinMaxResult ChessEngine::minMax(Position& position, short player, UINT16 depth)
	{

		auto result = MinMaxResult{};
		result.values.resize(MIN_MAX_VALUE_SIZE);

		result.values[VALUE] = -player * INF;

		if (depth == 0)
		{
			// TODO: Change Engine Player
			result.values[VALUE] = ChessEvaluation::evaluate(position, PLAYER_WHITE, EVAL_FT_STANDARD);
			
			return result;
		}

		auto moves = ChessValidation::getValidMoves(position, player);

		if (moves.empty())
		{
			// TODO: Change Engine Player
			result.values[VALUE] = ChessEvaluation::evaluate(position, PLAYER_WHITE, EVAL_FT_STANDARD);

			return result;
		}

		for (auto move : moves)
		{
			auto current_position = ChessValidation::applyMove(position, move);

			// Stellungen wegen 3x Stellungswiederholung überspringen
			if (m_repitionMap.isPositionAlreadyLocked(current_position)) continue;

			auto new_result = minMax(current_position, -player, depth - 1);

			if (new_result.values[VALUE] > result.values[VALUE] && player == PLAYER_WHITE || new_result.values[VALUE] < result.values[VALUE] && player == PLAYER_BLACK)
			{
				result.values[VALUE] = new_result.values[VALUE];
				result.best = move;
			}
			// if empty
			else if (new_result.values[VALUE] == result.values[VALUE] && result.best.startX == 0 && result.best.startY == 0 && result.best.targetX == 0 && result.best.targetY == 0)
			{
				result.best = move;
			}
		}
		return result;
	}

	MinMaxResult ChessEngine::alphaBeta(Position& position, short player, UINT16 depth, FLOAT alpha, FLOAT beta, BOOL sort, BOOL killer)
	{
		auto result = MinMaxResult{};		
		auto is_player_white = player == PLAYER_WHITE;

		result.values.resize(MIN_MAX_VALUE_SIZE);
		result.values[VALUE] = -player * INF;

		if (depth == 0)
		{
			// TODO: Change Engine Player
			result.values[VALUE] = ChessEvaluation::evaluate(position, PLAYER_WHITE, EVAL_FT_STANDARD);
			return result;
		} 

		auto start = START_CLOCK();
		MoveList moves = ChessValidation::getValidMoves(position, player);
		m_v1 += CALCULATE_MICROSECONDS(start);
		m_v2++;

		if (moves.empty())
		{
			result.values[VALUE] = ChessEvaluation::evaluate(position, PLAYER_WHITE, EVAL_FT_STANDARD);
			return result;
		}


		if (sort) sortMoves(&moves, position, depth, &result, killer);


		for (auto move : moves)
		{
			//auto current_position = ChessValidation::applyMove(position, move);
			
			position.applyMove(move);

			//// Stellungen wegen 3x Stellungswiederholung überspringen
			//if (m_repitionMap.isPositionAlreadyLocked(current_position)) continue;

			std::pair<FLOAT, FLOAT> alpha_beta = is_player_white ? 
				std::make_pair(result.values[VALUE], beta) : 
				std::make_pair(alpha, result.values[VALUE]);

			auto new_result = alphaBeta(position, -player, depth - 1, alpha_beta.first, alpha_beta.second, sort, killer);

			position.undoLastMove();

			if (killer)
			{
				for (INT32 d = 0; d <= depth; d++)
				{
					result.killers[d].merge(new_result.killers[d]);
				}
			}

			if (is_player_white && new_result.values[VALUE] > alpha)
			{
				result.values[VALUE] = new_result.values[VALUE];
				result.best = move;

				if (alpha >= beta) 
				{ 
					if (killer) result.killers[depth].insert(move);
					//m_count++;  
					break; 
				}
			}
			else if (!is_player_white && new_result.values[VALUE] < beta)
			{
				result.values[VALUE] = new_result.values[VALUE];
				result.best = move;

				if (beta <= alpha) 
				{
					if (killer) result.killers[depth].insert(move);
					//m_count++;
					break; 
				}
			}
		}

		
		return result;
	}

	MinMaxResult ChessEngine::nested(Position& position, short player, UINT16 depth, BOOL sort)
	{
		auto moves = ChessValidation::getValidMoves(position, player);
		//if (sort) sortMoves(&moves, position, depth, result, sort);
		
		MinMaxResult result;
		result.values.resize(NESTED_SIZE);

		if (position.getGameState() != GameState::Active)
		{
			auto value = ChessEvaluation::evaluate(position, PLAYER_WHITE, 0);

			result.values[VALUE] = value;
			result.values[LOWER] = value;
			result.values[UPPER] = value;

			return result;
		}

		auto is_player_white = player == PLAYER_WHITE;

		if (depth == 0 || moves.empty())
		{
			// TODO: Change Engine Player
			result.values[VALUE] = ChessEvaluation::evaluate(position, PLAYER_WHITE, EVAL_FT_STANDARD);
			result.values[LOWER] = -INF;
			result.values[UPPER] = INF;

			//if (is_player_white) result.values[ALPHA] = result.values[VALUE];
			//else result.values[BETA] = result.values[VALUE];

			return result;
		}

		result.values[VALUE] = -player * INF;
		result.values[LOWER] = -INF;
		result.values[UPPER] = -INF;

		for (auto move : moves)
		{
			auto current_position = ChessValidation::applyMove(position, move);
			auto new_result = nested(current_position, -player, depth - 1, sort);

			if (new_result.values[VALUE] > result.values[VALUE] && player == PLAYER_WHITE || new_result.values[VALUE] < result.values[VALUE] && player == PLAYER_BLACK)
			{
				result.values[VALUE] = new_result.values[VALUE];
				result.values[LOWER] = std::max(result.values[LOWER], -new_result.values[UPPER]);
				result.values[UPPER] = std::max(result.values[UPPER], -new_result.values[LOWER]);
				result.best = move;
			}
			else if (new_result.values[VALUE] == result.values[VALUE] && result.best.startX == 0 && result.best.startY == 0 && result.best.targetX == 0 && result.best.targetY == 0)
			{
				result.best = move;
			}
		}

		return result;
	}

	MinMaxResult ChessEngine::nestedAlphaBeta(Position& position, short player, UINT16 depth, FLOAT alpha, FLOAT beta, BOOL sort)
	{
		MinMaxResult result;
		result.values.resize(NESTED_SIZE);

		if (position.getGameState() != GameState::Active)
		{
			auto value = ChessEvaluation::evaluate(position, PLAYER_WHITE, 0);

			result.values[VALUE] = value;
			result.values[LOWER] = value;
			result.values[UPPER] = value;

			return result;
		}

		auto moves = ChessValidation::getValidMoves(position, player);
		//if (sort) sortMoves(&moves, position);

		auto is_player_white = player == PLAYER_WHITE;

		if (depth == 0 || moves.empty())
		{
			// TODO: Change Engine Player
			result.values[VALUE] = ChessEvaluation::evaluate(position, PLAYER_WHITE, EVAL_FT_STANDARD);
			result.values[LOWER] = -INF;
			result.values[UPPER] = INF;

			if (is_player_white) result.values[ALPHA] = result.values[VALUE];
			else result.values[BETA] = result.values[VALUE];

			return result;
		}

		result.values[VALUE] = -player * INF;
		result.values[LOWER] = -INF;
		result.values[UPPER] = -INF;
		result.values[ALPHA] = alpha;
		result.values[BETA] = beta;

		for (auto move : moves)
		{
			auto current_position = ChessValidation::applyMove(position, move);
			auto new_result = nestedAlphaBeta(current_position, -player, depth - 1, result.values[ALPHA], result.values[BETA], sort);

			if (is_player_white && new_result.values[VALUE] > result.values[ALPHA])
			{
				result.values[ALPHA] = new_result.values[VALUE];
				result.values[VALUE] = result.values[ALPHA];
				result.values[LOWER] = std::max(result.values[LOWER], -new_result.values[UPPER]);
				result.values[UPPER] = std::max(result.values[UPPER], -new_result.values[LOWER]);
				result.best = move;

				if (alpha > beta) {  break; }
			}
			else if (!is_player_white && new_result.values[VALUE] < result.values[BETA])
			{
				result.values[BETA] = new_result.values[VALUE];
				result.values[VALUE] = result.values[BETA];
				result.values[LOWER] = std::max(result.values[LOWER], -new_result.values[UPPER]);
				result.values[UPPER] = std::max(result.values[UPPER], -new_result.values[LOWER]);
				result.best = move;

				if (beta < alpha) {  break; }
			}
		}

		return result;
	}

	VOID ChessEngine::sortMoves(MoveList* moves, Position& position, UINT16 depth, MinMaxResult* pResult, BOOL killer)
	{

		std::sort(moves->begin(), moves->end(), [&position, depth, pResult, killer](const Move& left, const Move& right)
		{
			if (left.capture && !right.capture) return true;
			if (right.capture && !left.capture) return false;

			if (left.capture && right.capture)
			{
				return getCaptureValue(position[left.startY][left.startX], position[left.targetY][left.targetX]) > getCaptureValue(position[right.startY][right.startX], position[right.targetY][right.targetX]);
			}

			// Heuristics
			if (killer)
			{
				if (pResult->killers.size() > 0)
					std::cout << "huh" << std::endl;

				auto killer_left = killerHeuristics(left, depth, pResult);
				auto killer_right = killerHeuristics(right, depth, pResult);
				if (killer_left && !killer_right) return true;
				else if (!killer_left && killer_right) return false;
			}

			FLOAT left_move_value = ChessEvaluation::evaluate(ChessValidation::applyMove(position, left), -position.getPlayer(), 0);
			FLOAT right_move_value = ChessEvaluation::evaluate(ChessValidation::applyMove(position, right), -position.getPlayer(), 0);

			return left_move_value > right_move_value;
		});
	}

	BOOL ChessEngine::killerHeuristics(const Move& move, UINT16 depth, MinMaxResult* pResult)
	{
		if (pResult->killers.find(depth) != pResult->killers.end())
		{
			for (Move killer : pResult->killers[depth])
			{
				if (killer == move) { std::cout << "killer found\n";  return true; }
			}
		}
		return false;
	}

	owl::ChessEngine::Captures ChessEngine::getCaptureValue(CHAR attacker, CHAR victim)
	{
		attacker = std::tolower(attacker);
		victim = std::toupper(victim);

		switch (victim)
		{
		case WHITE_PAWN:
			switch (attacker)
			{
			case BLACK_PAWN: return Captures::pxP; break;
			case BLACK_KNIGHT: return Captures::nxP; break;
			case BLACK_BISHOP: return Captures::bxP; break;
			case BLACK_ROOK: return Captures::rxP; break;
			case BLACK_QUEEN: return Captures::qxP; break;
			case BLACK_KING: return Captures::kxP; break;
			}
			break;
		case WHITE_KNIGHT:
			switch (attacker)
			{
			case BLACK_PAWN: return Captures::pxN; break;
			case BLACK_KNIGHT: return Captures::nxN; break;
			case BLACK_BISHOP: return Captures::bxN; break;
			case BLACK_ROOK: return Captures::rxN; break;
			case BLACK_QUEEN: return Captures::qxN; break;
			case BLACK_KING: return Captures::kxN; break;
			}
			break;
		case WHITE_BISHOP:
			switch (attacker)
			{
			case BLACK_PAWN: return Captures::pxB; break;
			case BLACK_KNIGHT: return Captures::nxB; break;
			case BLACK_BISHOP: return Captures::bxB; break;
			case BLACK_ROOK: return Captures::rxB; break;
			case BLACK_QUEEN: return Captures::qxB; break;
			case BLACK_KING: return Captures::kxB; break;
			}
			break;
		case WHITE_ROOK:
			switch (attacker)
			{
			case BLACK_PAWN: return Captures::pxR; break;
			case BLACK_KNIGHT: return Captures::nxR; break;
			case BLACK_BISHOP: return Captures::bxR; break;
			case BLACK_ROOK: return Captures::rxR; break;
			case BLACK_QUEEN: return Captures::qxR; break;
			case BLACK_KING: return Captures::kxR; break;
			}
			break;
		case WHITE_QUEEN:
			switch (attacker)
			{
			case BLACK_PAWN: return Captures::pxQ; break;
			case BLACK_KNIGHT: return Captures::nxQ; break;
			case BLACK_BISHOP: return Captures::bxQ; break;
			case BLACK_ROOK: return Captures::rxQ; break;
			case BLACK_QUEEN: return Captures::qxQ; break;
			case BLACK_KING: return Captures::kxQ; break;
			}
			break;
		}

		return Captures::kxP;
	}

	std::string ChessEngine::getCaptureValueString(Captures capture)
	{
		return s_capture_map[static_cast<UINT64>(capture)];
	}
}