" This project setting will automatically open NERDTree

function! StartWithNERDTree()
        NERDTree
        " expand NERDTree
        normal O
        " switch to code window
        :wincmd w
endfunction

" open NERDTree window upon start
if exists(":NERDTree")
        autocmd VimEnter * call StartWithNERDTree()
endif

" ignore patterns
let NERDTreeIgnore=['\~$', '_vimrc_local.vim', 'build', 'waf']
set wildignore+=*.pyc,*~,*.o,*.so,*.class,*/.git/*,*/.hg/*,*/.svn/*,.DS_Store

" set tab and spaces
set expandtab
set smartindent

" indent settings for c/c++
setlocal softtabstop=4
setlocal shiftwidth=4
setlocal tabstop=4

" automatically read file when it is changed from the outside
set autoread

" automatically write when calling commands like :next and :make
set autowrite

" ignore case if search pattern is all lowercase, case-sensitive otherwise
set ignorecase
set smartcase


" mark the white spaces in file
set list
set listchars=tab:>.,trail:.,extends:#,nbsp:.
