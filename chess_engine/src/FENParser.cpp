#include "FENParser.hpp"
#include "ChessEngine.hpp"

#include <vector>
#include <string>
#include "ChessEvaluation.hpp"

namespace owl {

    Position FENParser::fenToPosition(std::string fen)
    {
        // TODO: Regex check for invalid fen-strings
        // Beispiel: rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1

        auto rows = std::vector<std::string>();

        // Erste Zeile bis vorletzte Zeile
        while (fen.find("/") != std::string::npos)
        {
            auto pos = fen.find("/");
            rows.push_back(fen.substr(0, pos));
            fen.replace(0, pos + 1, "");
        }
        // Letzte Zeile
        auto pos = fen.find(" ");
        rows.push_back(fen.substr(0, pos));
        fen.replace(0, pos + 1, "");

        auto data = BoardArray();

        INT32 y = 0;
        for (auto row : rows)
        {
            auto x = 0;
            for (auto c : row)
            {
                if (std::isdigit(c))
                {
                    auto digit = static_cast<INT32>(c-48);
                    for (auto i = 0; i < digit; i++)
                    {
                        data[y][x] = ' ';
                        x++;
                    }
                }
                else
                {
                    data[y][x] = c;
                    x++;
                }
            }
            y++;
        }

        // Spieler
        auto player = fen[0] == 'w' ? PLAYER_WHITE : PLAYER_BLACK;
        fen.replace(0, 2, "");

        // Rochade
        pos = fen.find(" ");
        auto castling_str = fen.substr(0, pos);
        fen.replace(0, pos + 1, "");

        auto white_castling_short   = castling_str.find("K") != std::string::npos;
        auto white_castling_long    = castling_str.find("Q") != std::string::npos;
        auto black_castling_short   = castling_str.find("k") != std::string::npos;
        auto black_castling_long    = castling_str.find("q") != std::string::npos;

        // En Passant Field
        BOOL en_passant = false;
        std::string passant_pos_str;
        std::pair<INT32, INT32> en_passant_pos = { 0,0 };
        if (fen[0] == '-')
        {
            fen.replace(0, 2, "");
        }
        else
        {
            pos = fen.find(" ");
            passant_pos_str = fen.substr(0, pos);
            fen.replace(0, pos + 1, "");
            en_passant = true;
        }

        if (en_passant)
        {
            en_passant_pos.first    = passant_pos_str[0] - 'a';
            en_passant_pos.second   = 7 - passant_pos_str[1] - '1';
        }

        // Halbz�ge
        pos = fen.find(" ");
        UINT16 ply_count = static_cast<UINT16>(std::stoi(fen.substr(0, pos)));
        fen.replace(0, pos + 1, "");

        // Zugnummer
        UINT16 move_number = static_cast<UINT16>(std::stoi(fen));

        return Position{data, player, 
            white_castling_short, white_castling_long, 
            black_castling_short, black_castling_long, 
            en_passant, en_passant_pos, 
            ply_count, move_number};
    }

    std::string FENParser::positionToFen(const Position& pos)
    {
        auto out = std::string();

        // 8x8 CHAR-Array
        for (INT32 r = 0; r < 8; r++)
        {
            auto count = 0;
            for (INT32 l = 0; l < 8; l++)
            {
                if (pos[r][l] == ' ')
                {
                    count++;
                }
                else
                {
                    if (count > 0)
                        out += std::to_string(count);
                    out += pos[r][l];
                    count = 0;
                }
            }
            if (count > 0)
                out += std::to_string(count);
            out += "/";
        }
        out.replace(out.size() - 1, 1, " ");

        // Spieler
        out += pos.getPlayer() == PLAYER_WHITE ? "w " : "b ";

        // Rochade
        if (pos.getWhiteCastlingShort())    out += "K";
        if (pos.getWhiteCastlingLong())     out += "Q";
        if (pos.getBlackCastlingShort())    out += "k";
        if (pos.getBlackCastlingLong())     out += "q";
        out += " ";

        // En Passant
        if (!pos.isEnPassant()) out += "- ";
        else
        {
            out += pos.getEnPassant().first + 'a';
            out += '8' - pos.getEnPassant().second;
            out += " ";
        }

        // Halbz�ge
        out += std::to_string(pos.getPlyCount());
        out += " ";

        // Zugnummer
        out += std::to_string(pos.getMoveNumber());

        return out;
    }

    Move FENParser::stringToMove(const std::string& algebraicNotation, const Position* position)
    {
        Move move;

        // capture vom String abfragen:
        move.capture = algebraicNotation.find("x") != std::string::npos;

        // promotion vom String abfragen:
        move.promotion = algebraicNotation.size() == 6 ? algebraicNotation[5] : 0;

        // position setzen
        move.startX = algebraicNotation[0] - 'a';
        move.startY = 7 - (algebraicNotation[1] - '1');
        move.targetX = algebraicNotation[3] - 'a';
        move.targetY = 7 - (algebraicNotation[4] - '1');

        // Relative Attribute des Zugs in Abh�ngigkeit zur Position auslesen.
        if (position != nullptr)
        {
            auto&& pos = *position;
            /// TODO: En Passant & Rochade checken 
            auto piece = pos[move.startY][move.startX];

            // Capture abfangen:
            move.capture = (pos[move.targetY][move.targetX] != EMPTY_PLACE);

            // Rochade abfangen:
            if ((piece == KING_WHITE || piece == KING_BLACK))
            {
                if (move.startX == KING_START_X && move.targetX == KING_CASTLING_LONG_X) move.castlingLong = true;
                else if (move.startX == KING_START_X && move.targetX == KING_CASTLING_SHORT_X) move.castlingShort = true;
            }

            // En Passant abfangen
            // Ansatz: Wenn der Zug diagonal ist und das Zielfeld leer ist, muss es sich um ein En Passant-Capture handeln
            if (pos.isEnPassant() 
                && pos[move.targetY][move.targetX] == EMPTY_PLACE 
                && move.startX != move.targetY
                && ((piece == PAWN_WHITE && move.startY == PAWN_DOUBLE_MOVE_TARGET_BLACK_Y) 
                    || (piece == PAWN_BLACK && move.startY == PAWN_DOUBLE_MOVE_TARGET_WHITE_Y)))
            {
                move.capture = true;
                move.enPassantCapture = true;
            }
        }

        return move;
    }

    std::string FENParser::moveToString(const Move& move)
    {
        std::string out = "";

        // start
        out += move.startX + 'a';
        out += '8' - move.startY;

        // capture
        out += move.capture ? 'x' : '-';

        // target
        out += move.targetX + 'a';
        out += '8' - move.targetY;

        // promotion
        out += move.promotion;

        return out;
    }
}