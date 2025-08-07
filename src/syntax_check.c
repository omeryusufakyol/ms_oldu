#include "../include/minishell.h"
#include "../libft/libft.h"

static int	is_operator(const char *token)
{
	return (!ft_strcmp(token, "|") ||
		!ft_strcmp(token, "<") ||
		!ft_strcmp(token, ">") ||
		!ft_strcmp(token, ">>") ||
		!ft_strcmp(token, "<<"));
}

int	check_syntax(char **tokens, t_ms *ms)
{
	int	i;

	if (!tokens || !tokens[0])
		return (0);
	if (is_operator(tokens[0]) && !is_quoted_operator(ms->raw_input, tokens[0]))
	{
		if (!ft_strcmp(tokens[0], "|"))
		{
			ft_putendl_fd("syntax error near unexpected token `|'", 2);
			ms->last_exit = 2;
			return (1);
		}
		else if (!tokens[1] || is_operator(tokens[1]))
		{
			// örn: `>`, `>>`, `<`, `<<` ama target yoksa → hata
			ft_putendl_fd("syntax error near unexpected token `newline'", 2);
			ms->last_exit = 2;
			return (1);
		}
	}
	i = 0;
	while (tokens[i])
	{
		if (is_operator(tokens[i]) && !is_quoted_operator(ms->raw_input, tokens[i]))
		{
			if (!tokens[i + 1])
			{
				ft_putendl_fd("syntax error near unexpected token `newline'", 2);
				ms->last_exit = 2;
				return (1);
			}
			if (is_operator(tokens[i + 1]) && ft_strcmp(tokens[i], "|"))
			{
				ft_putendl_fd("syntax error near unexpected token `newline'", 2);
				ms->last_exit = 2;
				return (1);
			}
		}
		i++;
	}
	return (0);
}
