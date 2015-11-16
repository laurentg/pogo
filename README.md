# POGO

A small POGO game simulator, based on the POGO board-game.

Un petit simulateur du jeu du même nom, basé sur le jeu de plateau POGO.

## License

This program is released in the public domain.
In place of a unreadable licence, here is a (SQLite-style) blessing:
- May you do good and not evil.
- May you find forgiveness for yourself and forgive others.
- May you share freely, never taking more than you give.

Ce programme est mis dans le domaine public.
Utilisez-le comme bon vous semble!

## Installation

In the pure KISS tradition:

	$ make
	$ su
	****
	$ cp -p pogo /usr/local/bin

## Rules

The game is played on a 3x3 board with 6 pieces by player
(black and white). The aim of the game is for a player to
control all the pieces of the board.

At the beginning of the game, the 6 pieces are placed as
done by the program (2x3 stacks of 2, each color one side
of the board).

Each turn, a player can move one of its stack. The player
controlling a stack is the player whose color is on the TOP
of the stack, regardless how many other colors are on the
stack. You can move 1, 2 or 3 pieces maximum from a stack
(the pieces on the top of it).
The move distance is given by the number of pieces moved: 
1 move (E, N, W, S) by piece, no more, no less.
You can jump over other stacks, go in any direction, and
move to an empty slot or another stack (you place the
moved pieces over the existing stack).

The game ends when a player control all the stacks on the
board.
	
## Authors

- Laurent GRÉGOIRE for the computer game (laurent.gregoire@gmail.com)
- Copyright (c) FilsFils international for the original POGO game.

