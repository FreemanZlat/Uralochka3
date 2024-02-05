import chess.pgn

# pgn = open("w0-self.pgn")
pgn = open("test-w4-w5.pgn")

count_draws = 0
count_white = 0
count_black = 0

draws = []

while True:
    print(count_draws + count_white + count_black)

    game = chess.pgn.read_game(pgn)
    if game is None:
        break

    try:
        ply_count = int(game.headers["PlyCount"])
        result = game.headers["Result"]
    except KeyError:
        continue

    if result == "1-0":
        count_white += 1
        continue
    elif result == "0-1":
        count_black += 1
        continue
    elif result == "1/2-1/2":
        count_draws += 1
    else:
        continue

    eval_min = 0
    eval_max = 0
    for pos in game.mainline():
        # print(pos.board().fen())

        comment = pos.comment
        if comment == "book" or comment == "":
            continue

        try:
            eval = int(100.0 * float(comment.split("/")[0]))
        except ValueError:
            continue

        if pos.turn() == chess.WHITE:
            eval = -eval

        eval_min = min(eval, eval_min)
        eval_max = max(eval, eval_max)

    draws.append({'min': eval_min, 'max': eval_max})

print(len(draws))

draws_max = sorted(draws, key=lambda x: x['max'], reverse=True)
draws_min = sorted(draws, key=lambda x: x['min'])

for idx in range(0, count_draws):
    print(f'{idx+1}/{count_draws} ({round(100.0*(idx+1)/count_draws, 2)}) : '
          f'{draws_max[idx]["max"]} -- {-draws_min[idx]["min"]}')

eval_white = draws_max[count_white]['max']
eval_black = -draws_min[count_black]['min']
eval_res = 0.5 * (eval_white + eval_black)
print(f'{eval_white} -- {eval_black} --- {eval_res}')
