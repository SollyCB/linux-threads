https://nullprogram.com/blog/2023/03/23/ allowed me to really understand how clone was really working.

In the initial commit, I just copy-pasted his newthread asm implementation, but tomorrow I am going to replace it with clone3, both for
demonstrating my own understanding by adding my own flair to someone else's work and so that I can mess with tls (it will be easier to
work with a greater number of arguments in asm with clone3 than clone).

# Playing with low level linux threads

No pthread for this guy! - to paraphrase Peter Griffin.
