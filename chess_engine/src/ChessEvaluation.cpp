#include "ChessEvaluation.hpp"
#include "ChessValidation.hpp"
#include "ChessEngine.hpp"
#include <unordered_map>

namespace matt
{
	float ChessEvaluation::evaluate(const Position& position, short enginePlayer, unsigned char evaluationFeatureFlags)
	{
		auto white_score = 0.00f;
		auto black_score = 0.00f;

		// Zur Bestimmung des einfachen Materials f�r Endspielstellung werden Bauernwerte erst zum Schluss addiert
		auto white_pawn_score = 0.00f;
		auto black_pawn_score = 0.00f;

		// Anzahl der Figuren
		unsigned short white_pawns = 0;
		unsigned short black_pawns = 0;
		unsigned short white_knights = 0;
		unsigned short black_knights = 0;
		unsigned short white_bishops = 0;
		unsigned short black_bishops = 0;
		unsigned short white_rooks = 0;
		unsigned short black_rooks = 0;
		unsigned short white_queens = 0;
		unsigned short black_queens = 0;

		// Datenstruktur f�r Piece Mobility
		std::unordered_map<char, int> possible_moves;

		possible_moves[PAWN_WHITE] = 0;
		possible_moves[PAWN_BLACK] = 0;
		possible_moves[KNIGHT_WHITE] = 0;
		possible_moves[KNIGHT_BLACK] = 0;
		possible_moves[BISHOP_WHITE] = 0;
		possible_moves[BISHOP_BLACK] = 0;
		possible_moves[ROOK_WHITE] = 0;
		possible_moves[ROOK_BLACK] = 0;
		possible_moves[QUEEN_WHITE] = 0;
		possible_moves[QUEEN_BLACK] = 0;
		possible_moves[KING_WHITE] = 0;
		possible_moves[KING_BLACK] = 0;

		// Pro Spielfeld Figure z�hlen und Materialwert hinzutragen
		// Anmerkung: Der Materialwert der Bauern wird sp�ter zu white_score bzw. black_score hinzugetragen!
		for (int y = 0; y < ROWS; y++)
		{
			for (int x = 0; x < COLUMNS; x++)
			{
				switch (position[y][x])
				{
				case PAWN_WHITE:
					white_pawns++;
					// Bauernstruktur f�r Wei� bestimmen
					if (evaluationFeatureFlags & EVAL_FT_PAWN_STRUCUTRE)
					{
						if (isDoublePawn(position, x, y))		white_pawn_score += PAWN_STRUCTURE_DOUBLE_PAWNS_PENALTY * PAWN_STRUCTURE_WEIGHT;
						if (isConnectedPawn(position, x, y))	white_pawn_score += PAWN_STRUCTURE_CONNECTED_PAWNS_BONUS * PAWN_STRUCTURE_WEIGHT;
						if (isChainPawn(position, x, y))		white_pawn_score += PAWN_STRUCTURE_CHAIN_PAWNS_BONUS * PAWN_STRUCTURE_WEIGHT;
						if (isIsolatedPawn(position, x, y))		white_pawn_score += PAWN_STRUCTURE_ISOLATED_PAWNS_PENALTY * PAWN_STRUCTURE_WEIGHT;
						if (isPassedPawn(position, x, y))		white_pawn_score += PAWN_STRUCTURE_PASSED_PAWNS_BONUS * PAWN_STRUCTURE_WEIGHT;
						if (isBackwardsPawn(position, x, y))	white_pawn_score += PAWN_STRUCTURE_BACKWARDS_PAWNS_PENALTY * PAWN_STRUCTURE_WEIGHT;
					}
					break;
				case PAWN_BLACK:
					black_pawns++;
					// Bauernstruktur f�r Schwarz bestimmen
					if (evaluationFeatureFlags & EVAL_FT_PAWN_STRUCUTRE)
					{
						if (isDoublePawn(position, x, y))		black_pawn_score += PAWN_STRUCTURE_DOUBLE_PAWNS_PENALTY * PAWN_STRUCTURE_WEIGHT;
						if (isConnectedPawn(position, x, y))	black_pawn_score += PAWN_STRUCTURE_CONNECTED_PAWNS_BONUS * PAWN_STRUCTURE_WEIGHT;
						if (isChainPawn(position, x, y))		black_pawn_score += PAWN_STRUCTURE_CHAIN_PAWNS_BONUS * PAWN_STRUCTURE_WEIGHT;
						if (isIsolatedPawn(position, x, y))		black_pawn_score += PAWN_STRUCTURE_ISOLATED_PAWNS_PENALTY * PAWN_STRUCTURE_WEIGHT;
						if (isPassedPawn(position, x, y))		black_pawn_score += PAWN_STRUCTURE_PASSED_PAWNS_BONUS * PAWN_STRUCTURE_WEIGHT;
						if (isBackwardsPawn(position, x, y))	black_pawn_score += PAWN_STRUCTURE_BACKWARDS_PAWNS_PENALTY * PAWN_STRUCTURE_WEIGHT;
					}
					break;
				case KNIGHT_WHITE:
					white_knights++; 
					white_score += MATERIAL_VALUES[KNIGHT];
					break;
				case KNIGHT_BLACK:
					black_knights++;
					black_score += MATERIAL_VALUES[KNIGHT];
					break;
				case BISHOP_WHITE:
					white_bishops++;
					white_score += MATERIAL_VALUES[BISHOP];
					break;
				case BISHOP_BLACK:
					black_bishops++;
					black_score += MATERIAL_VALUES[BISHOP];
					break;
				case ROOK_WHITE:
					white_rooks++;
					white_score += MATERIAL_VALUES[ROOK];
					break;
				case ROOK_BLACK:
					black_rooks++;
					black_score += MATERIAL_VALUES[ROOK];
					break;
				case QUEEN_WHITE:
					white_queens++;
					white_score += MATERIAL_VALUES[QUEEN];
					break;
				case QUEEN_BLACK:
					black_queens++;
					black_score += MATERIAL_VALUES[QUEEN];
					break;
				}
				// Piece Mobility Z�ge z�hlen
				if (evaluationFeatureFlags & EVAL_FT_PIECE_MOBILITY)
				{
					possible_moves[position[y][x]] += ChessValidation::countPossibleMovesOnField(position, x, y);
				}
			}
		}

		// Dynamischer Bonus f�r den Materialwert je Spielphase
		if (evaluationFeatureFlags & EVAL_FT_MATERIAL_DYNAMIC_GAME_PHASE)
		{
			// Spielphase bestimmen
			auto game_phase = position.getMoveNumber() >= MIDGAME_NUMBER ? GAME_PHASE_MID : GAME_PHASE_START;
			game_phase = game_phase == GAME_PHASE_MID && white_score + black_score <= MINIMUM_BALANCE_FOR_ENDGAME ? GAME_PHASE_END : game_phase;

			switch (game_phase)
			{
			case GAME_PHASE_START:
				white_score += white_pawns	 * MATERIAL_ADDITION_BEGIN_GAME_PHASE[PAWN]		* MATERIAL_DYNAMIC_GAME_PHASE_WEIGHT;
				white_score += white_knights * MATERIAL_ADDITION_BEGIN_GAME_PHASE[KNIGHT]	* MATERIAL_DYNAMIC_GAME_PHASE_WEIGHT;
				white_score += white_bishops * MATERIAL_ADDITION_BEGIN_GAME_PHASE[BISHOP]	* MATERIAL_DYNAMIC_GAME_PHASE_WEIGHT;
				white_score += white_rooks	 * MATERIAL_ADDITION_BEGIN_GAME_PHASE[ROOK]		* MATERIAL_DYNAMIC_GAME_PHASE_WEIGHT;
				white_score += white_queens  * MATERIAL_ADDITION_BEGIN_GAME_PHASE[QUEEN]	* MATERIAL_DYNAMIC_GAME_PHASE_WEIGHT;

				black_score += black_pawns   * MATERIAL_ADDITION_BEGIN_GAME_PHASE[PAWN]		* MATERIAL_DYNAMIC_GAME_PHASE_WEIGHT;
				black_score += black_knights * MATERIAL_ADDITION_BEGIN_GAME_PHASE[KNIGHT]	* MATERIAL_DYNAMIC_GAME_PHASE_WEIGHT;
				black_score += black_bishops * MATERIAL_ADDITION_BEGIN_GAME_PHASE[BISHOP]	* MATERIAL_DYNAMIC_GAME_PHASE_WEIGHT;
				black_score += black_rooks   * MATERIAL_ADDITION_BEGIN_GAME_PHASE[ROOK]		* MATERIAL_DYNAMIC_GAME_PHASE_WEIGHT;
				black_score += black_queens  * MATERIAL_ADDITION_BEGIN_GAME_PHASE[QUEEN]	* MATERIAL_DYNAMIC_GAME_PHASE_WEIGHT;
				break;
			case GAME_PHASE_MID:
				white_score += white_pawns	 * MATERIAL_ADDITION_MID_GAME_PHASE[PAWN]	* MATERIAL_DYNAMIC_GAME_PHASE_WEIGHT;
				white_score += white_knights * MATERIAL_ADDITION_MID_GAME_PHASE[KNIGHT] * MATERIAL_DYNAMIC_GAME_PHASE_WEIGHT;
				white_score += white_bishops * MATERIAL_ADDITION_MID_GAME_PHASE[BISHOP] * MATERIAL_DYNAMIC_GAME_PHASE_WEIGHT;
				white_score += white_rooks	 * MATERIAL_ADDITION_MID_GAME_PHASE[ROOK]	* MATERIAL_DYNAMIC_GAME_PHASE_WEIGHT;
				white_score += white_queens  * MATERIAL_ADDITION_MID_GAME_PHASE[QUEEN]	* MATERIAL_DYNAMIC_GAME_PHASE_WEIGHT;

				black_score += black_pawns   * MATERIAL_ADDITION_MID_GAME_PHASE[PAWN]	* MATERIAL_DYNAMIC_GAME_PHASE_WEIGHT;
				black_score += black_knights * MATERIAL_ADDITION_MID_GAME_PHASE[KNIGHT] * MATERIAL_DYNAMIC_GAME_PHASE_WEIGHT;
				black_score += black_bishops * MATERIAL_ADDITION_MID_GAME_PHASE[BISHOP] * MATERIAL_DYNAMIC_GAME_PHASE_WEIGHT;
				black_score += black_rooks   * MATERIAL_ADDITION_MID_GAME_PHASE[ROOK]	* MATERIAL_DYNAMIC_GAME_PHASE_WEIGHT;
				black_score += black_queens  * MATERIAL_ADDITION_MID_GAME_PHASE[QUEEN]	* MATERIAL_DYNAMIC_GAME_PHASE_WEIGHT;
				break;
			case GAME_PHASE_END:
				white_score += white_pawns	 * MATERIAL_ADDITION_END_GAME_PHASE[PAWN]	* MATERIAL_DYNAMIC_GAME_PHASE_WEIGHT;
				white_score += white_knights * MATERIAL_ADDITION_END_GAME_PHASE[KNIGHT] * MATERIAL_DYNAMIC_GAME_PHASE_WEIGHT;
				white_score += white_bishops * MATERIAL_ADDITION_END_GAME_PHASE[BISHOP] * MATERIAL_DYNAMIC_GAME_PHASE_WEIGHT;
				white_score += white_rooks	 * MATERIAL_ADDITION_END_GAME_PHASE[ROOK]	* MATERIAL_DYNAMIC_GAME_PHASE_WEIGHT;
				white_score += white_queens  * MATERIAL_ADDITION_END_GAME_PHASE[QUEEN]	* MATERIAL_DYNAMIC_GAME_PHASE_WEIGHT;

				black_score += black_pawns	 * MATERIAL_ADDITION_END_GAME_PHASE[PAWN]	* MATERIAL_DYNAMIC_GAME_PHASE_WEIGHT;
				black_score += black_knights * MATERIAL_ADDITION_END_GAME_PHASE[KNIGHT] * MATERIAL_DYNAMIC_GAME_PHASE_WEIGHT;
				black_score += black_bishops * MATERIAL_ADDITION_END_GAME_PHASE[BISHOP] * MATERIAL_DYNAMIC_GAME_PHASE_WEIGHT;
				black_score += black_rooks	 * MATERIAL_ADDITION_END_GAME_PHASE[ROOK]	* MATERIAL_DYNAMIC_GAME_PHASE_WEIGHT;
				black_score += black_queens  * MATERIAL_ADDITION_END_GAME_PHASE[QUEEN]	* MATERIAL_DYNAMIC_GAME_PHASE_WEIGHT;
				break;
			}
		}

		// Dynamische Bauern aktiviert?
		if (evaluationFeatureFlags & EVAL_FT_DYNAMIC_PAWNS)
		{
			// Bauernanzahl clampen:
			if (white_pawns > MAX_PAWN_COUNT) white_pawns = MAX_PAWN_COUNT;
			if (black_pawns > MAX_PAWN_COUNT) black_pawns = MAX_PAWN_COUNT;

			// Dynamische Bauern addieren
			white_pawn_score += white_pawns * MATERIAL_DYNAMIC_PAWNS[white_pawns];
			black_pawn_score += black_pawns * MATERIAL_DYNAMIC_PAWNS[black_pawns];
		}
		// Ansonsten normale Bauerneinheit addieren
		else
		{
			white_pawn_score += white_pawns * MATERIAL_VALUES[PAWN];
			black_pawn_score += black_pawns * MATERIAL_VALUES[PAWN];
		}

		// L�uferpaar aktiviert?
		if (evaluationFeatureFlags & EVAL_FT_BISHOP_PAIR)
		{
			if (white_bishops >= 2) white_score += BISHOP_PAIR_BONUS * BISHOP_PAIR_BONUS_WEIGHT;
			if (black_bishops >= 2) black_score += BISHOP_PAIR_BONUS * BISHOP_PAIR_BONUS_WEIGHT;
		}

		// Piece Mobility hinzuf�gen:
		if (evaluationFeatureFlags & EVAL_FT_PIECE_MOBILITY)
		{
			white_score += PIECE_MOBILITY_WEIGHT * PIECE_MOBILITY_PAWN_WEIGHT	* possible_moves[PAWN_WHITE];
			white_score += PIECE_MOBILITY_WEIGHT * PIECE_MOBILITY_KNIGHT_WEIGHT * possible_moves[KNIGHT_WHITE];
			white_score += PIECE_MOBILITY_WEIGHT * PIECE_MOBILITY_BISHOP_WEIGHT * possible_moves[BISHOP_WHITE];
			white_score += PIECE_MOBILITY_WEIGHT * PIECE_MOBILITY_ROOK_WEIGHT	* possible_moves[ROOK_WHITE];
			white_score += PIECE_MOBILITY_WEIGHT * PIECE_MOBILITY_QUEEN_WEIGHT	* possible_moves[QUEEN_WHITE];
			white_score += PIECE_MOBILITY_WEIGHT * PIECE_MOBILITY_KING_WEIGHT	* possible_moves[KING_WHITE];

			black_score += PIECE_MOBILITY_WEIGHT * PIECE_MOBILITY_PAWN_WEIGHT	* possible_moves[PAWN_BLACK];
			black_score += PIECE_MOBILITY_WEIGHT * PIECE_MOBILITY_KNIGHT_WEIGHT * possible_moves[KNIGHT_BLACK];
			black_score += PIECE_MOBILITY_WEIGHT * PIECE_MOBILITY_BISHOP_WEIGHT * possible_moves[BISHOP_BLACK];
			black_score += PIECE_MOBILITY_WEIGHT * PIECE_MOBILITY_ROOK_WEIGHT	* possible_moves[ROOK_BLACK];
			black_score += PIECE_MOBILITY_WEIGHT * PIECE_MOBILITY_QUEEN_WEIGHT	* possible_moves[QUEEN_BLACK];
			black_score += PIECE_MOBILITY_WEIGHT * PIECE_MOBILITY_KING_WEIGHT	* possible_moves[KING_BLACK];
		}

		// Bauern Materialwert addieren
		white_score += white_pawn_score;
		black_score += black_pawn_score;

		// Je nach Spieler score der Spieler voneinander abziehen und zur�ckgeben
		return enginePlayer == PLAYER_WHITE ? white_score - black_score : black_score - white_score;
	}
	bool ChessEvaluation::isDoublePawn(const Position& position, int x, int y)
	{
		if (isPieceEqualOnOffset(position, x, y, 0, 1)) return true;
		if (isPieceEqualOnOffset(position, x, y, 0, -1)) return true;

		return false;
	}
	bool ChessEvaluation::isConnectedPawn(const Position& position, int x, int y)
	{
		if (isPieceEqualOnOffset(position, x, y, 1, 0)) return true;		
		if (isPieceEqualOnOffset(position, x, y, -1, 0)) return true;

		return false;
	}
	bool ChessEvaluation::isIsolatedPawn(const Position& position, int x, int y)
	{
		if (isPieceEqualOnOffset(position, x, y, 1, 0)) return false;
		if (isPieceEqualOnOffset(position, x, y, -1, 0)) return false;
		if (isDoublePawn(position, x, y)) return false;
		if (isChainPawn(position, x, y)) return false;

		return true;
	}
	bool ChessEvaluation::isBackwardsPawn(const Position& position, int x, int y)
	{
		auto enemy_pawn = position.getPlayer() == PLAYER_WHITE ? PAWN_BLACK : PAWN_WHITE;
		auto dir = enemy_pawn == PAWN_WHITE ? 1 : -1;

		if (!isConnectedPawn(position, x, y))
		{
			// Gegnerischer Bauer vorne
			if (isPieceEnemyPawnOnOffset(position, x, y, 0, dir)) return true;
			if (isPieceEnemyPawnOnOffset(position, x, y, 1, 2 * dir)) return true;
			if (isPieceEnemyPawnOnOffset(position, x, y, -1, 2 * dir)) return true;
		}

		return false;
	}
	bool ChessEvaluation::isPassedPawn(const Position& position, int x, int y)
	{
		auto enemy_pawn = position[y][x] == PAWN_WHITE ? PAWN_BLACK : PAWN_WHITE;

		for (int i = 0; i < ROWS; i++)
		{
			if (i == y) continue; // Eigenes Feld ignorieren
			if (ChessValidation::isInsideChessboard(x, i))
			{
				if (position[i][x] == enemy_pawn) return false;
			}
		}

		return true;
	}
	bool ChessEvaluation::isChainPawn(const Position& position, int x, int y)
	{
		if (isPieceEqualOnOffset(position, x, y, 1, 1)) return true;
		if (isPieceEqualOnOffset(position, x, y, 1, -1)) return true;
		if (isPieceEqualOnOffset(position, x, y, -1, 1)) return true;
		if (isPieceEqualOnOffset(position, x, y, -1, -1)) return true;

		return false;
	}

	bool ChessEvaluation::isPieceEqualOnOffset(const Position& position, int x, int y, int xOffset, int yOffset)
	{
		if (ChessValidation::isInsideChessboard(x + xOffset, y + yOffset))
		{
			if (position[y][x] == position[y + yOffset][x + xOffset]) return true;
		}

		return false;
	}

	bool ChessEvaluation::isPieceEnemyPawnOnOffset(const Position& position, int x, int y, int xOffset, int yOffset)
	{
		auto startPawn = position.getPlayer() == PLAYER_WHITE ? PAWN_WHITE : PAWN_BLACK;
		auto targetPawn = startPawn == PAWN_WHITE ? PAWN_BLACK : PAWN_WHITE;

		if (ChessValidation::isInsideChessboard(x + xOffset, y + yOffset))
		{
			if (position[y][x] == startPawn && targetPawn == position[y + yOffset][x + xOffset]) return true;
		}

		return false;
	}
}