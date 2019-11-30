public void saveImageFile(String filename, StegoImage message)
{
    try
    {
        //retrieve image and write to file
        BufferedImage bi = message.getImage();
        File f = new File(filename);
        ImageIO.write(bi, "png", f);
    }
    catch (IOException e)
    {
        System.out.println("Error: " + e);
    }
}