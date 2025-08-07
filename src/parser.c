#include "../include/minishell.h"
#include "../libft/libft.h"

int	is_redirect(const char *token)
{
	return (!ft_strcmp(token, "<")
		|| !ft_strcmp(token, ">")
		|| !ft_strcmp(token, ">>")
		|| !ft_strcmp(token, "<<"));
}

char	*bash_quote_trim(const char *token, t_ms *ms)
{
	char	*res;
	int		i;
	int		j;
	char	quote;

	(void)ms;
	res = gc_malloc(ms, ft_strlen(token) + 1);
	if (!res)
	{
		write(2, "minishell: bash_quote_trim allocation failed\n", 34);
		return (NULL);
	}
	i = 0;
	j = 0;
	while (token[i])
	{
		if (token[i] == '\'' || token[i] == '"')
		{
			quote = token[i++];
			while (token[i] && token[i] != quote)
				res[j++] = token[i++];
			if (token[i] == quote)
				i++;
		}
		else
			res[j++] = token[i++];
	}
	res[j] = '\0';
	return (res);
}

static int	parse_redirect(t_cmd *cmd, char **tokens, int *i, t_ms *ms)
{
	int		fd;
	char	*clean;
	char	*filename;

	if (!ft_strcmp(tokens[*i], "<"))
	{
		filename = tokens[++(*i)];
		fd = open(filename, O_RDONLY);
		if (fd < 0)
		{
			perror(filename);
			ms->last_exit = 1;
			return (1);
		}
		close(fd);
		cmd->infile = gc_strdup(ms, filename);
	}
	else if (!ft_strcmp(tokens[*i], ">"))
	{
		cmd->outfile = gc_strdup(ms, tokens[++(*i)]);
		fd = open(cmd->outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if (fd < 0)
			perror(cmd->outfile);
		else
			close(fd);
		cmd->append = 0;
	}
	else if (!ft_strcmp(tokens[*i], ">>"))
	{
		cmd->outfile = gc_strdup(ms, tokens[++(*i)]);
		fd = open(cmd->outfile, O_WRONLY | O_CREAT | O_APPEND, 0644);
		if (fd < 0)
			perror(cmd->outfile);
		else
			close(fd);
		cmd->append = 1;
	}
	else if (!ft_strcmp(tokens[*i], "<<"))
	{
		clean = bash_quote_trim(tokens[*i + 1], ms);
		if (!clean)
		{
			ms->last_exit = 1;
			return (1);
		}
		add_heredoc(cmd, clean, ms);
		ms->heredoc_index++;
		cmd->heredoc = 1;
		(*i) += 2;
		return (0);
	}
	(*i)++;
	return (0);
}

int	is_quoted_operator(const char *raw_input, const char *op)
{
	int		i;
	int		inside_single = 0;
	int		inside_double = 0;
	size_t	op_len = ft_strlen(op);

	i = 0;
	while (raw_input[i])
	{
		if (raw_input[i] == '\'' && !inside_double)
			inside_single = !inside_single;
		else if (raw_input[i] == '"' && !inside_single)
			inside_double = !inside_double;
		else if (!inside_single && !inside_double
			&& !ft_strncmp(&raw_input[i], op, op_len))
			return (0); // dışarıda bulduk, quoted değil
		else if ((inside_single || inside_double)
			&& !ft_strncmp(&raw_input[i], op, op_len))
			return (1); // içeride bulduk, quoted

		i++;
	}
	return (0); // varsayılan: quoted değil
}

int is_quoted_operator_parser(const char *raw, int target_idx)
{
    int  i = 0;
    int  count = 0;
    int  start;
    char quote;

    while (raw[i])
    {
        // skip spaces
        while (raw[i] && (raw[i] == ' ' || raw[i] == '\t'))
            i++;

        if (!raw[i])
            break;

        start = i;
        // if it starts with a quote, consume until matching
        if (raw[i] == '\'' || raw[i] == '"')
        {
            quote = raw[i++];
            while (raw[i] && raw[i] != quote)
                i++;
            if (raw[i] == quote)
                i++;
        }
        else
        {
            // unquoted chunk, but it may contain quoted substrings
            while (raw[i] && raw[i] != ' ' && raw[i] != '\t')
            {
                if (raw[i] == '\'' || raw[i] == '"')
                {
                    quote = raw[i++];
                    while (raw[i] && raw[i] != quote)
                        i++;
                    if (raw[i] == quote)
                        i++;
                }
                else
                    i++;
            }
        }

        // when we reach the target token, test its first char
        if (count == target_idx)
            return (raw[start] == '\'' || raw[start] == '"');

        count++;
    }
    // default: not quoted
    return 0;
}

t_cmd	*parser(char **tokens, t_ms *ms)
{
	t_cmd	*cmds;
	t_cmd	*current;
	int		i;
	int		cmd_start;
	int		has_heredoc;

	cmds = NULL;
	i = 0;
	while (tokens[i])
	{
		current = init_cmd(ms);
		cmd_start = i;
		has_heredoc = 0;
		while (tokens[i])
		{
			if (!ft_strcmp(tokens[i], "|") && !is_quoted_operator_parser(ms->raw_input, i))
				break;

			// Gerçek redirect: işlem yapılır
			if (is_redirect(tokens[i]) && !is_quoted_operator_parser(ms->raw_input, i))
			{
				if (!ft_strcmp(tokens[i], "<<"))
					has_heredoc = 1;
				if (parse_redirect(current, tokens, &i, ms))
					return (NULL);
				continue;
			}
			// Quoted redirect VE başta: hata
			if (is_redirect(tokens[i]) && is_quoted_operator_parser(ms->raw_input, i) && i == cmd_start)
			{
				write(2, "minishell: ", 11);
				write(2, tokens[i], ft_strlen(tokens[i]));
				write(2, ": command not found\n", 21);
				ms->last_exit = 127;
				return (NULL);
			}
			// Diğer her şey: argüman sayılır
			i++;
		}
		current->args = copy_args(tokens, cmd_start, i, ms);
		if (has_heredoc || current->args)
			add_cmd(&cmds, current);
		if (tokens[i])
			i++;
	}
	return (cmds);
}
