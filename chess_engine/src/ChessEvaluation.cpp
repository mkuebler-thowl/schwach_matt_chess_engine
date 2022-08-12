#include "ChessEvaluation.hpp"
#include "ChessValidation.hpp"
#include "ChessEngine.hpp"
#include <unordered_map>

namespace owl
{
	FLOAT ChessEvaluation::evaluate(Position& position, short enginePlayer, UCHAR evaluationFeatureFlags)
	{
		// Ist die Position eine Endstellung?
		if ((position.getGameState() == GameState::PlayerBlackWins && enginePlayer == PLAYER_WHITE)
			|| position.getGameState() == GameState::PlayerWhiteWins && enginePlayer == PLAYER_BLACK) return -INF;
		if ((position.getGameState() == GameState::PlayerBlackWins && enginePlayer == PLAYER_BLACK)
			|| position.getGameState() == GameState::PlayerWhiteWins && enginePlayer == PLAYER_WHITE) return INF;
		if (position.getGameState() == GameState::Remis) return 0.00f;

		FLOAT score[PLAYER_COUNT] = { 0.0f };					// Score
		FLOAT extra_pawn_score[PLAYER_COUNT] = { 0.0f };		// Bauernscore (Wird nachtr�glich zum Score addiert)
		FLOAT square_table_score[PLAYER_COUNT] = { 0.0f };		// Square-Table Score

		std::pair<INT32, INT32> king_pos[PLAYER_COUNT] = {{0, 0}};

		UINT16 piece_count[PLAYER_COUNT][MAX_PIECE_TYPES] = { {0} };	 // Figurenanzahl
		UINT16 possible_moves[PLAYER_COUNT][MAX_PIECE_TYPES] = { {0} }; // Piece Mobility Count

		// Pro Spielfeld Figure z�hlen und Materialwert hinzutragen
		// Anmerkung: Der Materialwert der Bauern wird sp�ter zu white_score bzw. black_score hinzugetragen, 
		// da der Wert f�r die Berechnung, ob die Position eine Endstellung ist, nicht ber�cksichtig werden soll
		for (INT32 y = FIRST_ROW_INDEX; y < ROWS; y++)
		{
			for (INT32 x = FIRST_COLUMN_INDEX; x < COLUMNS; x++)
			{
				auto piece = position[y][x];

				// Leere Felder �berspringen:
				if (piece == EMPTY_FIELD)
					continue;

				auto type = GET_PIECE_INDEX_BY_TYPE(piece);
				auto color = GET_PLAYER_INDEX_BY_PIECE(piece);

				piece_count[color][type]++;

				// Bauernstruktur bestimmen
				if (evaluationFeatureFlags & EVAL_FT_PAWN_STRUCTURE && type == PAWN_INDEX)
				{
					auto is_double = isDoublePawn(position, x, y);
					auto is_connected = isConnectedPawn(position, x, y);
					auto is_chain = isChainPawn(position, x, y);
					auto is_passed = isPassedPawn(position, x, y);

					auto is_isolated = !(is_double && is_connected && is_chain);
					auto is_backwards = !is_connected ? isBackwardsPawn(position, x, y) : false;

					if (is_double)		extra_pawn_score[color] += PAWN_STRUCTURE_DOUBLE_PAWNS_PENALTY * PAWN_STRUCTURE_WEIGHT;
					if (is_connected)	extra_pawn_score[color] += PAWN_STRUCTURE_CONNECTED_PAWNS_BONUS * PAWN_STRUCTURE_WEIGHT;
					if (is_chain)		extra_pawn_score[color] += PAWN_STRUCTURE_CHAIN_PAWNS_BONUS * PAWN_STRUCTURE_WEIGHT;
					if (is_passed)		extra_pawn_score[color] += PAWN_STRUCTURE_PASSED_PAWNS_BONUS * PAWN_STRUCTURE_WEIGHT;
					
					if (is_isolated)	extra_pawn_score[color] += PAWN_STRUCTURE_ISOLATED_PAWNS_PENALTY * PAWN_STRUCTURE_WEIGHT;
					if (is_backwards)	extra_pawn_score[color] += PAWN_STRUCTURE_BACKWARDS_PAWNS_PENALTY * PAWN_STRUCTURE_WEIGHT;
				}

				score[color] += MATERIAL_VALUES[type];

				// Piece Mobility Z�ge z�hlen
				if (evaluationFeatureFlags & EVAL_FT_PIECE_MOBILITY)
				{
					possible_moves[color][type] += ChessValidation::countPossibleMovesOnField(position, x, y);
				}

				// Square Table addieren (Zun�chst ohne K�nig)
				if (evaluationFeatureFlags & EVAL_FT_PIECE_SQUARE_TABLE)
				{
					auto table_index = x + (COLUMNS * y);
					auto amount = 0.0f;

					switch (type)
					{
					case PAWN_INDEX:		amount += PIECE_SQUARE_TABLE_WEIGHT * PIECE_SQUARE_TABLE_PAWN_WEIGHT	* PIECE_SQUARE_TABLE_PAWN[table_index]; break;
					case KNIGHT_INDEX:	amount += PIECE_SQUARE_TABLE_WEIGHT * PIECE_SQUARE_TABLE_KNIGHT_WEIGHT	* PIECE_SQUARE_TABLE_KNIGHT[table_index]; break;
					case BISHOP_INDEX:	amount += PIECE_SQUARE_TABLE_WEIGHT * PIECE_SQUARE_TABLE_BISHOP_WEIGHT	* PIECE_SQUARE_TABLE_BISHOP[table_index]; break;
					case ROOK_INDEX:		amount += PIECE_SQUARE_TABLE_WEIGHT * PIECE_SQUARE_TABLE_ROOK_WEIGHT	* PIECE_SQUARE_TABLE_ROOK[table_index]; break;
					case QUEEN_INDEX:		amount += PIECE_SQUARE_TABLE_WEIGHT * PIECE_SQUARE_TABLE_QUEEN_WEIGHT	* PIECE_SQUARE_TABLE_QUEEN[table_index]; break;
					// Bestimme die Positionen der K�nige, um diese nach den Iterationen der Square-Table hinzuzuf�gen, 
					// da die Spielphase erst sp�ter bestimmt werden kann und es f�r Mittel- und Endspiel unterschiedliche Tabellen gibt
					case KING_INDEX: king_pos[color] = { x,y }; break;
					}
					square_table_score[color] += amount;
				}
			}
		}

		// Spielphase abfragen
		auto game_phase = position.getGamePhase();
		auto material = score[WHITE_INDEX] + score[BLACK_INDEX];

		// Spielphase bestimmen bzw. anpassen
		if (game_phase == GamePhase::Opening && material <= MAX_MATERIAL_SUM_MID_GAME)	position.enterNextGamePhase();
		if (game_phase == GamePhase::Mid && material <= MAX_MATERIAL_SUM_END_GAME)		position.enterNextGamePhase();
		
		// Dynamische Bauern aktiviert?
		if (evaluationFeatureFlags & EVAL_FT_DYNAMIC_PAWNS)
		{
			for (auto color = FIRST_PLAYER_INDEX; color < PLAYER_COUNT; color++)
			{
				auto index = std::min(piece_count[color][PAWN_INDEX], DYNAMIC_PAWNS_LAST_INDEX); // Grenzen einhalten
				extra_pawn_score[color] += piece_count[color][PAWN_INDEX] * MATERIAL_DYNAMIC_PAWNS[index];
			}
		}

		// Dynamischer Bonus f�r den Materialwert je Spielphase
		if (evaluationFeatureFlags & EVAL_FT_MATERIAL_DYNAMIC_GAME_PHASE)
		{
			for (INT32 color_index = FIRST_PLAYER_INDEX; color_index < PLAYER_COUNT; color_index++)
			{
				for (INT32 type_index = FIRST_PIECE_TYPES_INDEX; type_index < MAX_PIECE_TYPES; type_index++)
				{
					auto factor = 1.0f;
					switch (game_phase)
					{
					case GamePhase::Opening: factor = MATERIAL_ADDITION_BEGIN_GAME_PHASE[type_index]; break;
					case GamePhase::Mid: factor = MATERIAL_ADDITION_MID_GAME_PHASE[type_index]; break;
					case GamePhase::End: factor = MATERIAL_ADDITION_END_GAME_PHASE[type_index]; break;
					}
					score[color_index] += piece_count[color_index][type_index] * factor * MATERIAL_DYNAMIC_GAME_PHASE_WEIGHT;
				}
			}
		}

		// Piece Square Table f�r K�nig hinzuf�gen
		if (evaluationFeatureFlags & EVAL_FT_PIECE_SQUARE_TABLE && game_phase != GamePhase::Opening)
		{
			for (auto color_index = FIRST_PLAYER_INDEX; color_index < PLAYER_COUNT; color_index++)
			{
				auto table_index = king_pos[color_index].first + ((COLUMNS * king_pos[color_index].second));
				auto factor = 0.0f;
				switch (game_phase)
				{
				case GamePhase::Mid: factor = PIECE_SQUARE_TABLE_KING_MID_GAME_WEIGHT * PIECE_SQUARE_TABLE_KING_MID_GAME[table_index]; break;
				case GamePhase::End: factor = PIECE_SQUARE_TABLE_KING_END_GAME_WEIGHT * PIECE_SQUARE_TABLE_KING_END_GAME[table_index]; break;
				}
				square_table_score[color_index] += piece_count[color_index][KING_INDEX] * factor;
			}
		}

		// L�uferpaar aktiviert?
		if (evaluationFeatureFlags & EVAL_FT_BISHOP_PAIR)
		{
			for (auto color_index = FIRST_PLAYER_INDEX; color_index < PLAYER_COUNT; color_index++)
			{
				if (piece_count[color_index][BISHOP_INDEX] >= MIN_BISHOP_COUNT_PRECONDITION_BONUS)
					score[color_index] += BISHOP_PAIR_BONUS * BISHOP_PAIR_BONUS_WEIGHT;
			}
		}

		// Piece Mobility hinzuf�gen:
		if (evaluationFeatureFlags & EVAL_FT_PIECE_MOBILITY)
		{
			for (INT32 color_index = FIRST_PLAYER_INDEX; color_index < PLAYER_COUNT; color_index++)
			{
				for (INT32 type_index = FIRST_PIECE_TYPES_INDEX; type_index < MAX_PIECE_TYPES; type_index++)
				{
					auto factor = 1.0f;
					switch (type_index)
					{
					case PAWN_INDEX: factor = PIECE_MOBILITY_PAWN_WEIGHT; break;
					case KNIGHT_INDEX: factor = PIECE_MOBILITY_KNIGHT_WEIGHT; break;
					case BISHOP_INDEX: factor = PIECE_MOBILITY_BISHOP_WEIGHT; break;
					case ROOK_INDEX: factor = PIECE_MOBILITY_ROOK_WEIGHT; break;
					case QUEEN_INDEX: factor = PIECE_MOBILITY_QUEEN_WEIGHT; break;
					case KING_INDEX: factor = PIECE_MOBILITY_KING_WEIGHT; break;
					}
					score[color_index] += PIECE_MOBILITY_WEIGHT * factor * possible_moves[color_index][type_index];
				}
			}
		}

		// Materialwerte summieren
		for (INT32 color_index = FIRST_PLAYER_INDEX; color_index < PLAYER_COUNT; color_index++)
		{
			score[color_index] += extra_pawn_score[color_index] + square_table_score[color_index];
		}

		// Je nach Spieler score der Spieler voneinander abziehen und zur�ckgeben
		auto&& final_score = enginePlayer == PLAYER_WHITE ?
			score[WHITE_INDEX] - score[BLACK_INDEX] : score[BLACK_INDEX] - score[WHITE_INDEX];

		return final_score;
	}

	BOOL ChessEvaluation::isDoublePawn(const Position& position, INT32 x, INT32 y)
	{
		if (isPieceEqualOnOffset(position, x, y, 0, 1)) return true;
		if (isPieceEqualOnOffset(position, x, y, 0, -1)) return true;

		return false;
	}
	BOOL ChessEvaluation::isConnectedPawn(const Position& position, INT32 x, INT32 y)
	{
		if (isPieceEqualOnOffset(position, x, y, 1, 0)) return true;		
		if (isPieceEqualOnOffset(position, x, y, -1, 0)) return true;

		return false;
	}

	BOOL ChessEvaluation::isBackwardsPawn(const Position& position, INT32 x, INT32 y)
	{
		// Hinweis: isConnectedPawn(...) muss vorher false sein
		auto dir = position.getPlayer();

		// Gegnerischer Bauer vorne
		if (isPieceEnemyPawnOnOffset(position, x, y, 0, dir)) return true;
		if (isPieceEnemyPawnOnOffset(position, x, y, 1, 2 * dir)) return true;
		if (isPieceEnemyPawnOnOffset(position, x, y, -1, 2 * dir)) return true;

		return false;
	}
	BOOL ChessEvaluation::isPassedPawn(const Position& position, INT32 x, INT32 y)
	{
		auto enemy_pawn = GetEnemyPiece(position.getPlayer(), PAWN_INDEX);

		for (INT32 i = 0; i < ROWS; i++)
		{
			if (i == y) continue; // Eigenes Feld ignorieren
			if (ChessValidation::isInsideChessboard(x, i))
			{
				if (position[i][x] == enemy_pawn) return false;
			}
		}

		return true;
	}
	BOOL ChessEvaluation::isChainPawn(const Position& position, INT32 x, INT32 y)
	{
		if (isPieceEqualOnOffset(position, x, y, 1, 1)) return true;
		if (isPieceEqualOnOffset(position, x, y, 1, -1)) return true;
		if (isPieceEqualOnOffset(position, x, y, -1, 1)) return true;
		if (isPieceEqualOnOffset(position, x, y, -1, -1)) return true;

		return false;
	}

	BOOL ChessEvaluation::isPieceEqualOnOffset(const Position& position, INT32 x, INT32 y, INT32 xOffset, INT32 yOffset)
	{
		if (ChessValidation::isInsideChessboard(x + xOffset, y + yOffset))
		{
			if (position[y][x] == position[y + yOffset][x + xOffset]) return true;
		}

		return false;
	}
	BOOL ChessEvaluation::isPieceEnemyPawnOnOffset(const Position& position, INT32 x, INT32 y, INT32 xOffset, INT32 yOffset)
	{
		auto start_color = GetPlayerIndexByPositionPlayer(position.getPlayer());
		auto target_color = start_color + 1 % PLAYER_COUNT;

		auto start_pawn = PIECES[start_color][PAWN_INDEX];
		auto target_pawn = PIECES[target_color][PAWN_INDEX];

		if (ChessValidation::isInsideChessboard(x + xOffset, y + yOffset))
		{
			if (position[y][x] == start_pawn && target_pawn == position[y + yOffset][x + xOffset]) return true;
		}

		return false;
	}
	UCHAR ChessEvaluation::GetPlayerIndexByPositionPlayer(short currentPlayerOfPosition)
	{
		return currentPlayerOfPosition == PLAYER_WHITE ? WHITE_INDEX : BLACK_INDEX;
	}
	UCHAR ChessEvaluation::GetEnemyPiece(short currentPlayerOfPosition, UINT16 pieceIndex)
	{
		auto&& enemy_color = GetPlayerIndexByPositionPlayer(-currentPlayerOfPosition);
		return PIECES[enemy_color][pieceIndex];
	}
}