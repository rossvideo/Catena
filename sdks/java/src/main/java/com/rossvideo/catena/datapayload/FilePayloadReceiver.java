package com.rossvideo.catena.datapayload;

import java.io.File;

public class FilePayloadReceiver extends AbstractFilePayloadReceiver
{
    private File workingDirectory;
    private String defaultPrefix;
    
    public FilePayloadReceiver(File workingDirectory, String defaultPrefix)
    {
        this.workingDirectory = workingDirectory;
        this.defaultPrefix = defaultPrefix;
    }

    private File findFirstUnusedFile(String prefix)
    {
        File file = new File(workingDirectory, prefix);
        int i = 1;
        while (file.exists())
        {
            file = new File(workingDirectory, prefix + " " + i);
            i++;
        }
        return file;
    }
    
    public String getDefaultPrefix()
    {
        return defaultPrefix;
    }
    
    @Override
    public File getFileFor(String fileName)
    {
        if (fileName != null && !fileName.isEmpty())
        {
            fileName.replace('/', ' ');
            fileName.replace('\\', ' ');
            if (fileName.startsWith(".."))
            {
                fileName = fileName.substring(2);
            }
            return new File(workingDirectory, fileName);
        }

        if (defaultPrefix != null && !defaultPrefix.isEmpty())
        {
            return findFirstUnusedFile(getDefaultPrefix());
        }
        
        return null;
    }

}
