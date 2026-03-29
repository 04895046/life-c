# life-c
csc209 final project

rules:
a dead cell with 3 neighbours becomes alive, taking the majority of the 3 neighbours color.
a cell with two neighbours stays alive, maintaining the color of its neighbours.
a cell with (one or less) or (four or more) neighbours dies.

on game start, players may place {x} cells each on a {n x n} board.
for each turn, each player can choose to place {y} new cells, convert {y} existing cells, or remove {y} cells from their opponent
end condition: {z_1, z_2}

