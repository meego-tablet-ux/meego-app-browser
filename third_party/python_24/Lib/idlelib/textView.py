"""Simple text browser for IDLE

"""

from Tkinter import *
import tkMessageBox

class TextViewer(Toplevel):
    """
    simple text viewer dialog for idle
    """
    def __init__(self, parent, title, fileName, data=None):
        """If data exists, load it into viewer, otherwise try to load file.

        fileName - string, should be an absoulute filename
        """
        Toplevel.__init__(self, parent)
        self.configure(borderwidth=5)
        self.geometry("=%dx%d+%d+%d" % (625, 500,
                                        parent.winfo_rootx() + 10,
                                        parent.winfo_rooty() + 10))
        #elguavas - config placeholders til config stuff completed
        self.bg = '#ffffff'
        self.fg = '#000000'

        self.CreateWidgets()
        self.title(title)
        self.transient(parent)
        self.grab_set()
        self.protocol("WM_DELETE_WINDOW", self.Ok)
        self.parent = parent
        self.textView.focus_set()
        #key bindings for this dialog
        self.bind('<Return>',self.Ok) #dismiss dialog
        self.bind('<Escape>',self.Ok) #dismiss dialog
        if data:
            self.textView.insert(0.0, data)
        else:
            self.LoadTextFile(fileName)
        self.textView.config(state=DISABLED)
        self.wait_window()

    def LoadTextFile(self, fileName):
        textFile = None
        try:
            textFile = open(fileName, 'r')
        except IOError:
            tkMessageBox.showerror(title='File Load Error',
                    message='Unable to load file %r .' % (fileName,))
        else:
            self.textView.insert(0.0,textFile.read())

    def CreateWidgets(self):
        frameText = Frame(self, relief=SUNKEN, height=700)
        frameButtons = Frame(self)
        self.buttonOk = Button(frameButtons, text='Close',
                               command=self.Ok, takefocus=FALSE)
        self.scrollbarView = Scrollbar(frameText, orient=VERTICAL,
                                       takefocus=FALSE, highlightthickness=0)
        self.textView = Text(frameText, wrap=WORD, highlightthickness=0,
                             fg=self.fg, bg=self.bg)
        self.scrollbarView.config(command=self.textView.yview)
        self.textView.config(yscrollcommand=self.scrollbarView.set)
        self.buttonOk.pack()
        self.scrollbarView.pack(side=RIGHT,fill=Y)
        self.textView.pack(side=LEFT,expand=TRUE,fill=BOTH)
        frameButtons.pack(side=BOTTOM,fill=X)
        frameText.pack(side=TOP,expand=TRUE,fill=BOTH)

    def Ok(self, event=None):
        self.destroy()

if __name__ == '__main__':
    #test the dialog
    root=Tk()
    Button(root,text='View',
            command=lambda:TextViewer(root,'Text','./textView.py')).pack()
    root.mainloop()
