# The Obelix Manifesto #

##Why

Most every (euro) kid from my generation knows Obelix. Obelix, one of the protagonists in the *Asterix* series of French comic albums, is a friendly Gaul (Frenchman :-), living in a little village at the edge of Gaul (France :-) - the last little bit of territory in Gaul not yet occupied by Caesar's Roman legions in 50 BCE. The reason the village hasn't been overrun yet is that the village's druid knows the recipe of a magic potion giving incredible force and power to whoever drinks it. 

Obelix now fell into a cauldron with this magic potion as a child, and since then is perpetually strong - he doesn't need top-ups, so to say. Now an adult, he's a menhir cutter. Menhirs are massive, upright stones, probably used for some religious purpose, which can still be found in the landscape where Asterix and Obelix supposedly lived - the Breton area in France (footnote: these menhirs are a bit of a historical lie by the writers of Asterix: they were probably erected by people living in Europe about 4000 years before the Celtic Gauls did). Obelix takes his job very seriously, but he's also a bit of a traditionalist - he cuts all his menhirs identically, he cuts them all by hand, and he moves them all by hand.

Which brings me to this software project. I decided to approach this project like Obelix approaches menhir cutting: use as little tools as necessary, tools that are needed need to be simple and must have proven their worth, and innovations in design are not required. And I've always wanted to write a general-purpose programming language, so that's what this project is. Look at it this way: some people have a woodworking workshop in the basement, where they make wooden furniture that is not necessarily as good looking or the same quality as you would get in a furniture store, but it's what they do. They get enjoyment and entertainment out of it. I don't work wood at night in my basement, I code. Because I like doing it.

As with anything in life, things change over time. This project started out as an interpreted Python-like language executed in a homecooked virtual machine. Parsing was handled by ``lexa``, a ``lex``/``flex`` like lexer, and ``panoramix``, a ``yacc``/``bison`` type LR1 parser generator. It differed from those ancient tools in that instead of generating a huge ``c`` file implementing the parser and all the actions it triggers, it generated a smaller parser skeleton that called out the actions using the ``dl`` library. 

The basic design principles of Obelix are:
* Obelix is general-purpose scripting library, like for example python.
* It will be written in C++. True Gallic conservatism would require C89, but we're not completely behind the times, so we'll use C++20.

## What

