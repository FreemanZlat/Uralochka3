import chess.pgn
import math

pgn = open("3_42.pgn")

count_draws = 0
count_white = 0
count_black = 0

stats = []

while True:
    game = chess.pgn.read_game(pgn)
    if game is None:
        break

    try:
        result = game.headers["Result"]
    except KeyError:
        continue

    res = 0
    if result == "1-0":
        count_white += 1
        res = 1
    elif result == "0-1":
        count_black += 1
        res = -1
    elif result == "1/2-1/2":
        count_draws += 1
        res = 0
    else:
        continue

    print(f"d{count_draws} + w{count_white} + b{count_black} = {count_draws + count_white + count_black}")

    ply = 0
    for pos in game.mainline():
        comment = pos.comment
        if comment == "book" or comment == "":
            continue

        try:
            eval = int(100.0 * float(comment.split("/")[0]))
        except ValueError:
            continue

        if pos.turn() == chess.WHITE:
            eval = -eval

        stats.append({'eval': eval, 'res': res})
        if eval != 0:
            stats.append({'eval': -eval, 'res': -res})

        ply += 1
        if ply > 100:
            break

    # if count_draws == 1000:
    #     break

print(f"stats len: {len(stats)}")

SCALE = 5

stats_sorted = sorted(stats, key=lambda x: x['eval'])
graph = {}
for stat in stats_sorted:
    idx = math.floor((stat['eval'] + SCALE/2) / SCALE)
    if idx not in graph:
        graph[idx] = { 'sum': 0, "cnt": 0 }
    graph[idx]['sum'] += stat['res']
    graph[idx]['cnt'] += 1
for idx, val in graph.items():
    val['sum'] /= val['cnt']
    print(f"{idx*SCALE} : {val['sum']:.4f} - ({val['cnt']})")


