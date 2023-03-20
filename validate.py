def validate(y_hat):
	y = [1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1]

	import math
	errs = [yy - int(yyh) for yy, yyh in zip(y, y_hat.split(' '))]
	err = 0
	for e in errs:
		err += abs(e)

	err = err/len(y)
	return err
